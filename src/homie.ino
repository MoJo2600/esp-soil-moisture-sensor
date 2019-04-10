#include <Homie.h>
#include "Wire.h"

const int SLEEP_DURATION = 900000000; // 5min 300000000, 30 sek 30000000
HomieNode moistureNode("humidity", "humidity");
HomieNode batteryNode("battery", "battery");
HomieNode temperatureNode("temperature", "temperature");

const int PIN_CLK   = D5;
const int PIN_SOIL  = A0; 
const int PIN_LED   = D7;
const int PIN_SWITCH = D8;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;
unsigned long time_now = 0;

// Determined by using getrawvalues
const int MOISTURE_0 = 805;
const int MOISTURE_100 = 622;

const int BATTERY_1_6 = 512;
const int BATTERY_3_2 = 1024;

void setupHandler() {
  moistureNode.setProperty("unit").send("%");
  temperatureNode.setProperty("unit").send("°C");
  batteryNode.setProperty("unit").send("V");
}

void getSendMoisture() {
  digitalWrite(PIN_SWITCH, HIGH);
  
  int moisture = 0;
  int total = 0;
  int rawVal = 0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
    Homie.getLogger() << "Soil raw value: " << rawVal << endl;
  }

  moisture = map((total/sampleCount), MOISTURE_0, MOISTURE_100, 0, 100);

  Homie.getLogger() << "Moisture: " << moisture << endl;
  moistureNode.setProperty("value").send(String(moisture));
}

void getSendBattery() {
  digitalWrite(PIN_SWITCH, LOW);
  
  int total = 0;
  int rawVal = 0;
  int batteryRaw = 0;
  int sampleCount = 3;
  float batteryVoltage = 0.0;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
  }

  batteryVoltage = (map((total/sampleCount), BATTERY_1_6, BATTERY_3_2, 160, 320) / 100.0);
  
  Homie.getLogger() << "Battery: " << batteryVoltage << " V" << endl;
  batteryNode.setProperty("value").send(String(batteryVoltage));
}

void getSendTemperature() {
  float temperature = 0.0;
  int rawtmp = 0;
  
  // Begin transmission
  Wire.beginTransmission(TMP_ADDR);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();
  
  time_now = millis();
  while(millis() < time_now + 500){
    //wait approx. 500 ms
  }
  
  // Request 2 bytes , Msb first
  Wire.requestFrom(TMP_ADDR, 2 );
  // Read temperature as Celsius (the default)
  while(Wire.available()) {  
    int msb = Wire.read();
    int lsb = Wire.read();
    Wire.endTransmission();
    rawtmp = msb << 8 | lsb;
  }
 
  int value = rawtmp >> 4;
  temperature = value * 0.0625;

  Homie.getLogger() << "Temperature: " << temperature << " °C" << endl;
  temperatureNode.setProperty("value").send(String(temperature));
}

//void loopHandler() {
//}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:    
      Homie.getLogger() << "MQTT connected, doing stuff..." << endl;
      getSendMoisture();
      getSendTemperature();
      getSendBattery();   
      Homie.getLogger() << "Finished stuff, preparing for deep sleep..." << endl;
      Homie.prepareToSleep();
      break;
    case HomieEventType::READY_TO_SLEEP:
      Homie.getLogger() << "Ready to sleep" << endl;
      ESP.deepSleep(SLEEP_DURATION);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;

  Serial << "Entering Setup" << endl;
 
  Homie_setFirmware("awesome-soil-moisture", "1.0.0");
  Homie.setSetupFunction(setupHandler);
  //.setLoopFunction(loopHandler);

  moistureNode.advertise("unit");
  moistureNode.advertise("percent");

  batteryNode.advertise("unit");
  batteryNode.advertise("volts");

  temperatureNode.advertise("unit");
  temperatureNode.advertise("degrees");
  
  Homie.onEvent(onHomieEvent);
  Homie.setup();

  Wire.begin();

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);

  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_SWITCH, HIGH);

  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte
  Wire.write(0x60);             // sends potentiometer value byte
  Wire.endTransmission();     // stop transmitting
  
  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);
}

void loop() {
  Homie.loop();
}
