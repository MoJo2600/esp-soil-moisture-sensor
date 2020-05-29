/* Soil Moisture Monitor.
  
  Hardware ESP8266 Soil Moisture Probe V2.2 (NodeMcu1.0).
  https://wiki.aprbrother.com/en/ESP_Soil_Moisture_Sensor.html

  Implemented Homie https://homieiot.github.io/ to send messages to MQTT

  Made by Christian Erhardt 2019/04/21
  Based on the great work of Ve2Cuz Real Drouin - https://www.qsl.net/v/ve2cuz//garden/

  ///////// Pin Assigment ///////

  A0  Input Soil Moisture and Battery
  GPIO4   SDA for tmp112
  GPIO5   SCL for tmp112
  GPIO12  Button S1 (For Homie configuration reset)
  GPIO13  LED
  GPIO14  Clock Output for soil moisture sensor
  GPIO15  SWITCH for measuring Soil Moisture or Battery Voltage

  //////////////////////////////////////////////////////////////////////
*/
#include <Arduino.h>
#include <Homie.h>
#include <Wire.h>
#include <Timer.h>
#include <sensor.h>
#include <wizard.h>
#include <constants.h>

#define FW_NAME "esp-soil-moisture-sensor"
#define FW_VERSION "2.0.0"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

// homie node
HomieNode sensorNode("soilsensor", "SoilSensor", "soilsensor");

// homie settings
HomieSetting<bool> startCalibrationSetting("startCalibration", "When checked, the device will start a calibration wizard at http://soilsensor.local");
HomieSetting<bool> useLEDSetting("useLED", "Defines if the LED should be active");
HomieSetting<long> sleepDurationSetting("sleepDuration", "The sleep duration in minutes (Maximum 71 minutes)");
HomieSetting<long> dryReadingAt3VSetting("dryReadingAt3V", "Sensor reading dry at 3V VCC");
HomieSetting<long> wetReadingAt3VSetting("wetReadingAt3V", "Sensor reading submerged in water at 3V VCC");
HomieSetting<long> batteryFullSetting("batteryFull", "ADC reading of full battery");
HomieSetting<long> batteryEmptySetting("batteryEmpty", "ADC reading of empty battery");

// Timer to prepare to sleep
Timer sleepTimer;

bool run_wizard = false;

/*
* Function: prepareToSleep
* ------------------------
* Callback to tell homie to prepare to sleep
*/
void prepareSleep() {
  Homie.prepareToSleep();
}

/* 
 * Function: getSendMoisture
 * -------------------------
 * This function reads the moisture sensor multiple times and takes the average of the readings.
 * It then publishes this value via MQTT.
 * 
 * batteryCharge: The current battery charge
 * 
 * Since the reading is related to the battery voltage, a correction is applied to compensate
 * for this.
 */
void getSendMoisture(int batteryCharge) {
  
  SensorReading moistureReading = getMoisture(batteryCharge, dryReadingAt3VSetting.get(), wetReadingAt3VSetting.get(), batteryFullSetting.get());

  Homie.getLogger() << "Moisture percent: " << moistureReading.percent << endl;
  Homie.getLogger() << "Moisture adjusted: " << moistureReading.adjusted << endl;
  Homie.getLogger() << "Moisture raw: " << moistureReading.raw << endl;

  sensorNode.setProperty("moisture").send(String(moistureReading.percent));
  sensorNode.setProperty("moisture_adjusted").send(String(moistureReading.adjusted));
  sensorNode.setProperty("moisture_raw").send(String(moistureReading.raw));
}

/*
 * Function: getSendBattery
 * ------------------------
 * This function reads the battery voltage multiple times and takes the average of the readings.
 * It then publishes this value via MQTT.
 *  
 * returns: The current battery charge in percent
 */
int getSendBattery() {
  struct SensorReading reading = getBattery(batteryEmptySetting.get(), batteryFullSetting.get());

  Homie.getLogger() << "Battery: " << reading.percent << " %" << endl;
  Homie.getLogger() << "Battery adjusted: " << reading.adjusted << endl;
  Homie.getLogger() << "Battery raw: " << reading.raw << endl;

  sensorNode.setProperty("battery").send(String(reading.percent));
  sensorNode.setProperty("battery_adjusted").send(String(reading.adjusted));
  sensorNode.setProperty("battery_raw").send(String(reading.raw));

  return reading.adjusted;
}

/* 
 * Function: getSendTemperature
 * ----------------------------
 * This function reads the temprature over i2c bus.
 * It then publishes this value via MQTT.
 */
void getSendTemperature() {
  float temperature = 0.0;
  
  // Begin transmission
  Wire.beginTransmission(TMP_ADDR);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();
 
  // Request 2 bytes , Msb first to get temperature
  Wire.requestFrom(TMP_ADDR, 2);
  // Read temperature as Celsius (the default)
  if(Wire.available() == 2) {
    int msb = Wire.read();
    int lsb = Wire.read();

    int rawtmp = msb << 8 | lsb;
    int value = rawtmp >> 4;
    temperature = value * 0.0625;
  }
 
  #ifdef DEBUG
  Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
  #endif

  sensorNode.setProperty("temperature").send(String(temperature));
}

/*
 * Function: onHomieEvent
 * ----------------------
 * This function handles homie events.
 * To finish all tasks before deep sleep, we are waiting for the MQTT_READY
 * event. Then we will read all sensors and publish the values. After this we
 * call `prepareToSleep()`. When homie has finished all tasks, the event
 * READY_TO_SLEEP is raised. This is when we send the controller to sleep for
 * the configured time.
 */
void onHomieEvent(const HomieEvent& event) {
  float batteryCharge = 0.0;
  switch(event.type) {
    case HomieEventType::MQTT_READY:    
      Serial << "MQTT connected, doing stuff..." << endl;
      batteryCharge = getSendBattery();
      nonBlockingDelay(200);
      getSendMoisture(batteryCharge);
      nonBlockingDelay(200);
      getSendTemperature();
      Serial << "Finished, preparing for deep sleep..." << endl;
      sleepTimer.after(100, prepareSleep);
      break;
    case HomieEventType::READY_TO_SLEEP:
      Serial << "Ready to sleep" << endl;
      Homie.doDeepSleep(sleepDurationSetting.get() * 60 * 1000 * 1000);
      break;
  }
}

/*
 * Function: setup
 * ---------------
 * Default arduino setup handler. We prepare the whole environment here.
 */
void setup() {
  Serial.begin(74880);
  delay(10);
  Serial << endl << endl;
  Serial << "Entering Setup" << endl;

  // Prepare pins
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  // Set GPIO16 (=D0) pin mode to allow for deep sleep
  // Connect D0 to RST for this to work.
  pinMode(D0, WAKEUP_PULLUP);
  // initialize pin states
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_SWITCH, LOW);
  digitalWrite(PIN_BUTTON, HIGH);

  // Setup I2C library
  Wire.begin();

  initializeTemperatureSensor();
  
  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);

  Homie.setLoggingPrinter(&Serial);

  // Set up default values for settings
  sleepDurationSetting.setDefaultValue(DEFAULT_DEEP_SLEEP_MINUTES)
                      .setValidator([] (long candidate) {
                        // 72 Minutes is the maximum sleep time supported
                        // by ESP8266 https://thingpulse.com/max-deep-sleep-for-esp8266/
                        return candidate > 0 && candidate <= 72;
                      });

  useLEDSetting.setDefaultValue(DEFAULT_USE_LED);

  startCalibrationSetting.setDefaultValue(DEFAULT_START_CALIBRATION);

  dryReadingAt3VSetting.setDefaultValue(DEFAULT_MOIST_DRY_READING_AT_3V)
              .setValidator([] (long candidate) {
                return candidate > 0 && candidate <= 1024;
              });

  wetReadingAt3VSetting.setDefaultValue(DEFAULT_MOIST_WET_READING_AT_3V)
              .setValidator([] (long candidate) {
                return candidate > 0 && candidate <= 1024;
              });
              
  batteryFullSetting.setDefaultValue(DEFAULT_BATTERY_FULL_ADC_READING)
              .setValidator([] (long candidate) {
                return candidate > 0 && candidate <= 1024;
              });

  batteryEmptySetting.setDefaultValue(DEFAULT_BATTERY_EMPTY_ADC_READING)
              .setValidator([] (long candidate) {
                return candidate > 0 && candidate <= 1024;
              });


  if (Homie.isConfigured()) {
    #ifdef DEBUG
    Serial << "Homie is configured!" << endl;
    #endif

    // Should we start the calibration wizard?
    if (startCalibrationSetting.get()) {
      #ifdef DEBUG
      Serial << "Starting calibration wizard webserver! " << endl;
      #endif
      run_wizard = true;
      setup_wizard();
      return;
    }
  } else {
    #ifdef DEBUG
    Serial << "Homie is NOT configured!" << endl;
    #endif
  }

  Homie_setFirmware(FW_NAME, FW_VERSION);

  // Configure homie to use the build in button for configuration reset
  // Press and hold button for 2sec to reset the homie configuration
  Homie.setResetTrigger(PIN_BUTTON, LOW, 2000);

  // Advertise properties
  sensorNode.advertise("moisture")
            .setName("Moisture")
            .setDatatype("integer")
            .setUnit("%");
  sensorNode.advertise("moisture_adjusted")
            .setName("Moisture adjusted")
            .setDatatype("integer")
            .setUnit("#");
  sensorNode.advertise("moisture_raw")
            .setName("Moisture RAW value")
            .setDatatype("integer")
            .setUnit("#");  
  sensorNode.advertise("temperature")
            .setName("Temperature")
            .setDatatype("float")
            .setUnit("°C");
  sensorNode.advertise("battery")
            .setName("Battery")
            .setDatatype("integer")
            .setUnit("%");
  sensorNode.advertise("battery_adjusted")
            .setName("Battery adjusted")
            .setDatatype("integer")
            .setUnit("#");  
  sensorNode.advertise("battery_raw")
            .setName("Battery RAW value")
            .setDatatype("integer")
            .setUnit("#");  

  // Define event handler
  Homie.onEvent(onHomieEvent);

  // Set LED Pin for status messages, if LED is enabled
  if (useLEDSetting.get()) {
    Homie.setLedPin(PIN_LED, 1);
  } else {
    Homie.disableLedFeedback();
  }

  // Setup homie
  Homie.setup();
}

/*
 * Function: loop
 * --------------
 * Default arduino loop function. Call homie loop. 
 */
void loop() {
  if (run_wizard) {
    wizard_loop();
  } else {
    Homie.loop();
    sleepTimer.update();
  }
}
