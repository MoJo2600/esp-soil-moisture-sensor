
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
  GPIO12  Button S1 (Active Access Point for Config Sensor)
  GPIO13  LED
  GPIO14  Clock Output for soil moisture sensor
  GPIO15  SWITCH for measuring Soil Moisture or Battery Voltage

  //////////////////////////////////////////////////////////////////////
*/

#include <Arduino.h>
#include <Homie.h>
#include <Wire.h>

// sleep time in microseconds
const int SLEEP_DURATION = 15 * 60 * 1000 * 1000; // @TODO: Change to Homie setting

// homie nodes
HomieNode moistureNode("humidity", "humidity");
HomieNode batteryNode("battery", "battery");
HomieNode temperatureNode("temperature", "temperature");

// Pin settings
const int PIN_CLK    = D5;
const int PIN_SOIL   = A0; 
const int PIN_LED    = D7;
const int PIN_SWITCH = D8;
const int PIN_BUTTON = D6;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;
unsigned long time_now = 0;

/* 
This function reads the oisture sensor multiple times and takes the average of the readings.
Since the reading is related to the battery voltage, a correction is applied to compensate
for this.
*/
void getSendMoisture(float batteryCharge) {
  digitalWrite(PIN_SWITCH, HIGH);
  
  int moisture = 0;
  int total = 0;
  int rawVal = 0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
  }

  moisture = (100 * (total/sampleCount) / batteryCharge); // Battery Drop Correction
  moisture = map(moisture, 60, 82, 100, 0); // Convert to 0 - 100%, 0=Dry, 100=Wet
  if (moisture > 100) moisture = 100;
  if (moisture <  0) moisture = 0;

  Homie.getLogger() << "Moisture: " << moisture << endl;
  moistureNode.setProperty("value").send(String(moisture));
}

/* 
This function reads the battery voltage multiple times and takes the average of the readings.
The function will return the current battery charge in percent.
*/
float getSendBattery() {
  digitalWrite(PIN_SWITCH, LOW);
  
  int total = 0;
  int rawVal = 0;
  int sampleCount = 3;
  float batteryCharge = 0.0;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
  }

  batteryCharge = map((total/sampleCount), 736, 880, 0, 100); // 736 = 2.5v , 880 = 3.0v , esp dead at 2.3v
  if (batteryCharge > 100) batteryCharge = 100;
  if (batteryCharge < 0) batteryCharge = 0;
  
  Homie.getLogger() << "Battery: " << batteryCharge << " %" << endl;
  batteryNode.setProperty("value").send(String(batteryCharge));

  return batteryCharge;
}

/* 
This function reads the temprature over i2c bus.
*/
void getSendTemperature() {
  float temperature = 0.0;
  int rawtmp = 0;
  
  // Begin transmission
  Wire.beginTransmission(TMP_ADDR);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();
  
  // time_now = millis();
  // while(millis() < time_now + 500){
  //   //wait approx. 500 ms without stopping the cpu
  // }
  
  // Request 2 bytes , Msb first
  Wire.requestFrom(TMP_ADDR, 2 );
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
  temperatureNode.setProperty("value").send(String(temperature));
}

void onHomieEvent(const HomieEvent& event) {
  float batteryCharge = 0.0;
  switch(event.type) {
    case HomieEventType::MQTT_READY:    
      Serial << "MQTT connected, doing stuff..." << endl;
      batteryCharge = getSendBattery();
      getSendMoisture(batteryCharge);
      getSendTemperature();
      Serial << "Finished stuff, preparing for deep sleep..." << endl;
      Homie.prepareToSleep();
      break;
    case HomieEventType::READY_TO_SLEEP:
      Serial << "Ready to sleep" << endl;
      ESP.deepSleep(SLEEP_DURATION);
      break;
  }
}

void setup() {
  Serial.begin(74880);
  Serial << endl << endl;
  Serial << "Entering Setup" << endl;

  // Workaround for bug https://github.com/homieiot/homie-esp8266/issues/351
  if (Homie.isConfigured()) {
    WiFi.disconnect();
  }
 
  Homie_setFirmware("esp-soil-moisture-sensor", "1.0.1");
  // Configure homie to use the build in button for configuration reset
  // Press and hold button for 2sec to reset the homie configuration
  Homie.setResetTrigger(PIN_BUTTON, LOW, 5000);
  // Set LED Pin for status messages
  Homie.setLedPin(PIN_LED, 1); // @TODO Add homie setting to disable led to save power

  moistureNode.advertise("percent");
  batteryNode.advertise("percent");
  temperatureNode.advertise("degrees");
  
  Homie.setLoggingPrinter(&Serial);
  Homie.onEvent(onHomieEvent);
  Homie.setup();

  Wire.begin();

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
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
