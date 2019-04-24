
/* Soil Moisture Monitor.
   
  Hardware ESP8266 Soil Moisture Probe V2.2 (NodeMcu1.0).
  https://wiki.aprbrother.com/en/ESP_Soil_Moisture_Sensor.html

  Implemented Homie https://homieiot.github.io/ to send messages to MQTT

  Made by 2019/04/21 Christian Erhardt
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

//#define DEBUG

// sleep time in microseconds
const int DEFAULT_DEEP_SLEEP_MINUTES = 15;
const bool DEFAULT_USE_LED = true;

// homie nodes
HomieNode sensorNode("soilsensor", "SoilSensor", "soilsensor");

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


void nonBlockingDelay(int waitmillis) {
  time_now = millis();
  while(millis() < time_now + waitmillis) {
    //wait without stopping the cpu
  }
}

/*
Reads the sensor connected to pin PIN_SENSOR
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
This function reads the oisture sensor multiple times and takes the average of the readings.
Since the reading is related to the battery voltage, a correction is applied to compensate
for this.
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
This function reads the battery voltage multiple times and takes the average of the readings.
The function will return the current battery charge in percent.
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
This function reads the temprature over i2c bus.
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
      ESP.deepSleep(temperatureIntervalSetting.get() *  1000 * 1000); // 60 *
      break;
  }
}

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

  // Set LED Pin for status messages
  if (useLEDSetting.get()) {
    Homie.setLedPin(PIN_LED, 1);
  }

  // Advertise properties
  sensorNode.advertise("humidity")
            .setDatatype("int")
            .setUnit("%");
  sensorNode.advertise("temperature")
            .setDatatype("int")
            .setUnit("°C");
  sensorNode.advertise("battery")
            .setDatatype("int")
            .setUnit("%");
  
  // Define event handler
  Homie.onEvent(onHomieEvent);
  Homie.setup();

  Wire.begin();

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);

  // Set GPIO16 (=D0) pin mode to allow for deep sleep
  // Connect D0 to RST for this to work.
  // @TODO: Check if this is really needed
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

void loop() {
  Homie.loop();
}
