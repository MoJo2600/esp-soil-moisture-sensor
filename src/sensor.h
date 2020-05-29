#ifndef SENSOR_H
#define SENSOR_H

struct SensorReading {
    int raw,
        adjusted,
        percent;
};

/*
 * Function: nonBlockingDelay
 * --------------------------
 * A delay function that will not block the cpu like delay() does.
 */
void nonBlockingDelay(int waitmillis);

/*
 * Function: initializeTemperatureSensor
 * --------------------------
 * This method will initialize the temperature sensor
 */
void initializeTemperatureSensor();

/*
 * Function: readSensor
 * --------------------
 * Takes SAMPLE_COUNT readings from the sensor and returns the average
 */
int readSensor();

/* 
 * Function: getMoisture
 * -------------------------
 * This function returns the current moisture
 */
SensorReading getMoisture(int batteryCharge, int dryReading, int wetReading, int batteryFull);

/* 
 * Function: getBattery
 * -------------------------
 * This function returns the current battery reading
 */
SensorReading getBattery(int batteryEmpty, int batteryFull);

/* 
 * Function: getMoisture
 * -------------------------
 * This function returns the current temperature in °C
 */

float getTemperature();

#endif // SENSOR_H