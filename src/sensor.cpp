#include <Arduino.h>
#include <Wire.h>
#include <sensor.h>
#include <constants.h>

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

/*
 * Function: initializeTemperatureSensor
 * --------------------------
 * This method will initialize the temperature sensor
 */
void initializeTemperatureSensor() {
  // device TMP_ADDR is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
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
int readSensor(int PIN_SENSOR) {
  int total = 0;
  int rawVal = 0;
  int ret = 0;

  for(int i = 0; i < SAMPLE_COUNT; i++){
    rawVal = analogRead(PIN_SENSOR);
    total += rawVal;
    nonBlockingDelay(50);
  }

  ret = int((float)total / (float)SAMPLE_COUNT);

  return ret;
}

/* 
 * Function: getMoisture
 * -------------------------
 * This function reads the moisture sensor.
 * 
 * batteryCharge: The current battery charge
 * 
 * Since the reading is related to the battery voltage, a correction is applied to compensate
 * for this.
 */
SensorReading getMoisture(int batteryCharge, int dryReading, int wetReading) {
  // Connect Moisture sensor to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, HIGH);
  nonBlockingDelay(200);

  int moist_raw = readSensor(PIN_SENSOR);
  int moisture_adjusted = BATTERY_FULL_RAW * moist_raw / batteryCharge;

  struct SensorReading reading;
  reading.adjusted = moisture_adjusted;
  
  //reading.percent = 100 - (100 / (dryReading - wetReading) * (moisture_adjusted - wetReading));
  reading.percent = 100 - map(moisture_adjusted, wetReading, dryReading, 0, 100);
  reading.percent = reading.percent > 100 ? 100 : reading.percent;
  reading.percent = reading.percent < 0 ? 0 : reading.percent;

  reading.raw = moist_raw;

  return reading;
}

/*
 * Function: getSendBattery
 * ------------------------
 * This function reads the battery voltage multiple times and takes the average of the readings.
 * It then publishes this value via MQTT.
 *  
 * returns: The current battery charge in percent
 */
SensorReading getBattery() {
  // Connect Battery to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, LOW);
  nonBlockingDelay(200);

  int battery_raw = readSensor(PIN_SENSOR);
  int battery_percent = map(battery_raw, 800, 960, 0, 100);
  battery_percent = constrain(battery_percent, 0, 100); 

  struct SensorReading reading;
  reading.percent = battery_percent;
  // no adjustment needed for battery
  reading.adjusted = battery_raw;
  reading.raw = battery_raw;

  return reading;
}

/* 
 * Function: getSendTemperature
 * ----------------------------
 * This function reads the temprature over i2c bus.
 * It then publishes this value via MQTT.
 */
float getTemperature() {
  uint8_t temp[2];
  int16_t tempc;

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