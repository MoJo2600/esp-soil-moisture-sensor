#include <Arduino.h>
#include <Homie.h>
#include <Wire.h>

// if you want to have verbose output, uncomment this:
//#define DEBUG

// sleep time in microseconds
const int DEFAULT_DEEP_SLEEP_MINUTES = 15;
const bool DEFAULT_USE_LED = true;

// homie node
HomieNode sensorNode("soilsensor", "SoilSensor", "soilsensor");

// homie settings
HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The sleep duration in minutes");
HomieSetting<bool> useLEDSetting("useLED", "Defines if the LED should be active");

// Pin settings
const int PIN_CLK    = D5;
const int PIN_SENSOR   = A0; 
const int PIN_LED    = D7;
const int PIN_SWITCH = D8;
const int PIN_BUTTON = D6;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;
unsigned long time_now = 0;

/*
 * Function: nonBlockingDelay
 * --------------------------
 * A delay function that will not block the cpu like delay() does.
 */
void nonBlockingDelay(int waitmillis) {
  time_now = millis();
  while(millis() < time_now + waitmillis) {
    //wait without stopping the cpu
  }
}

/*
 * Function: readSensor
 * --------------------
 * Reads the sensor connected to pin PIN_SENSOR
 * Since the moisture sensor and the battery voltage are
 * connected to the same input pin, this function can be used
 * for both tasks.
 */
float readSensor() {
  float total = 0.0;
  float rawVal = 0.0;
  float ret = 0.0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SENSOR);
    #ifdef DEBUG
    Homie.getLogger() << "Raw Value: " << rawVal << endl;
    #endif 
    total += rawVal;
    nonBlockingDelay(50);
  }

  ret = total / sampleCount;

  #ifdef DEBUG
  Homie.getLogger() << "Result: " << ret << endl;
  #endif 

  return ret;
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
void getSendMoisture(float batteryCharge) {
  // Connect Moisture sensor to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, HIGH);
  nonBlockingDelay(200);

  int moisture = 0;
  float moist_raw = readSensor();

  moisture = (100 * moist_raw / batteryCharge); // Battery Drop Correction
  #ifdef DEBUG
  Homie.getLogger() << "Moisture after battery correction: " << moisture << endl;
  #endif
  moisture = map(moisture, 640, 830, 100, 0); // Convert to 0 - 100%, 0=Dry, 100=Wet
  #ifdef DEBUG
  Homie.getLogger() << "Moisture after mapping: " << moisture << endl;
  #endif
  if (moisture > 100) moisture = 100;
  if (moisture <  0) moisture = 0;

  Homie.getLogger() << "Moisture: " << moisture << endl;
  // moistureNode.setProperty("percent").send(String(moisture));
  sensorNode.setProperty("moisture").send(String(moisture));
}

/*
 * Function: getSendBattery
 * ------------------------
 * This function reads the battery voltage multiple times and takes the average of the readings.
 * It then publishes this value via MQTT.
 *  
 * returns: The current battery charge in percent
 */
float getSendBattery() {
  // Connect Battery to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, LOW);
  nonBlockingDelay(200);

  int batteryCharge = 0;
  float battery_raw = readSensor();

  batteryCharge = map(battery_raw, 736, 880, 0, 100); // 736 = 2.5v , 880 = 3.0v , esp dead at 2.3v
  #ifdef DEBUG
  Homie.getLogger() << "Moisture after mapping: " << batteryCharge << endl;
  #endif
  if (batteryCharge > 100) batteryCharge = 100;
  if (batteryCharge < 0) batteryCharge = 0;
  
  Homie.getLogger() << "Battery: " << batteryCharge << " %" << endl;
  sensorNode.setProperty("battery").send(String(batteryCharge));

  return batteryCharge;
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
 
  // Request 2 bytes , Msb first
  Wire.requestFrom(TMP_ADDR, 2);
  // Read temperature as Celsius (the default)
  while(Wire.available()) {
    int msb = Wire.read();
    int lsb = Wire.read();
    Wire.endTransmission();

    int rawtmp = msb << 8 | lsb;
    int value = rawtmp >> 4;
    temperature = value * 0.0625;
  }
 
  Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
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
      Serial << "Finished stuff, preparing for deep sleep..." << endl;
      Homie.prepareToSleep();
      break;
    case HomieEventType::READY_TO_SLEEP:
      Serial << "Ready to sleep" << endl;
      ESP.deepSleep(temperatureIntervalSetting.get() * 60 * 1000 * 1000);
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
  Serial << endl << endl;
  Serial << "Entering Setup" << endl;

  Homie.setLoggingPrinter(&Serial);

  temperatureIntervalSetting.setDefaultValue(DEFAULT_DEEP_SLEEP_MINUTES)
                            .setValidator([] (long candidate) {
                              return candidate > 0 && candidate < 72;
                            });

  useLEDSetting.setDefaultValue(DEFAULT_USE_LED);

  // Workaround for bug https://github.com/homieiot/homie-esp8266/issues/351
  if (Homie.isConfigured()) {
    #ifdef DEBUG
    Serial << "Homie is configured!" << endl;
    #endif
    WiFi.disconnect();
  } else {
    #ifdef DEBUG
    Serial << "Homie is NOT configured!" << endl;
    #endif
  }
 
  Homie_setFirmware("esp-soil-moisture-sensor", "1.0.1");

  // Configure homie to use the build in button for configuration reset
  // Press and hold button for 2sec to reset the homie configuration
  Homie.setResetTrigger(PIN_BUTTON, LOW, 2000);

  // Set LED Pin for status messages, if LED is enabled
  if (useLEDSetting.get()) {
    Homie.setLedPin(PIN_LED, 1);
  }

  // Advertise properties
  sensorNode.advertise("humidity")
            .setName("Humudity")
            .setDatatype("integer")
            .setUnit("%");
  sensorNode.advertise("temperature")
            .setName("Temperature")
            .setDatatype("float")
            .setUnit("°C");
  sensorNode.advertise("battery")
            .setName("Battery")
            .setDatatype("integer")
            .setUnit("%");
  
  // Define event handler
  Homie.onEvent(onHomieEvent);
  // Setup homie
  Homie.setup();

  // Setup I2C library
  Wire.begin();

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);

  // Set GPIO16 (=D0) pin mode to allow for deep sleep
  // Connect D0 to RST for this to work.
  pinMode(D0, WAKEUP_PULLUP);

  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_SWITCH, HIGH);
  digitalWrite(PIN_BUTTON, HIGH);

  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));           // sends instruction byte
  Wire.write(0x60);                 // sends potentiometer value byte
  Wire.endTransmission();           // stop transmitting
  
  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);
}

/*
 * Function: loop
 * --------------
 * Default arduino loop function. Call homie loop. 
 */
void loop() {
  Homie.loop();
}
