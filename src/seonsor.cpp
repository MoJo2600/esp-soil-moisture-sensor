#include <Arduino.h>
#include <Wire.h>

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
    yield();
  }
}

void initializeTemperatureSensor(int address) {
  // device address is specified in datasheet
  Wire.beginTransmission(address); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));           // sends instruction byte
  Wire.write(0x60);                 // sends potentiometer value byte
  Wire.endTransmission();           // stop transmitting
}

/*
 * Function: readSensor
 * --------------------
 * Reads the sensor connected to pin PIN_SENSOR
 * Since the moisture sensor and the battery voltage are
 * connected to the same input pin, this function can be used
 * for both tasks.
 */
int readSensor(int sensor_pin) {
  int total = 0;
  int rawVal = 0;
  int ret = 0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(sensor_pin);
    #ifdef DEBUG
    Homie.getLogger() << "Raw Value: " << rawVal << endl;
    #endif 
    total += rawVal;
    nonBlockingDelay(50);
  }

  ret = int((float)total / (float)sampleCount);

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
int getMoisture(int switch_pin, int sensor_pin) {
  // Connect Moisture sensor to the Pin via on PCB switch
  digitalWrite(switch_pin, HIGH);
  nonBlockingDelay(200);

  int moisture = 0;
  int moist_raw = readSensor(sensor_pin);
  return moist_raw;
}

/*
 * Function: getSendBattery
 * ------------------------
 * This function reads the battery voltage multiple times and takes the average of the readings.
 * It then publishes this value via MQTT.
 *  
 * returns: The current battery charge in percent
 */
int getBattery(int switch_pin, int sensor_pin) {
  // Connect Battery to the Pin via on PCB switch
  digitalWrite(switch_pin, LOW);
  nonBlockingDelay(200);

//   int batteryCharge = 0;
  int battery_raw = readSensor(sensor_pin);

  return battery_raw;
}



/* 
 * Function: getSendTemperature
 * ----------------------------
 * This function reads the temprature over i2c bus.
 * It then publishes this value via MQTT.
 */
float getTemperature(int address) {
  uint8_t temp[2];
  int16_t tempc;

  // Begin transmission
  Wire.beginTransmission(address);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();
 
  // Request 2 bytes , Msb first to get temperature
  Wire.requestFrom(address, 2);
  // Read temperature as Celsius (the default)
  if(Wire.available() == 2) {
    temp[0] = Wire.read();
    temp[1] = Wire.read();

		// Convert the data to 12-bits
    // ignore the lower 4 bits of byte 2
    temp[1]  = temp[1]  >> 4; 
    // combine to make one 12 bit binary number
    tempc = ((temp[0] << 4) | temp[1]);

    return round((tempc * 0.0625) * 100.0) / 100.0;
  }
}