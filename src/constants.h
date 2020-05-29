#include <Arduino.h>

#ifndef CONSTANTS_H
#define CONSTANTS_H

// DEFAULT SETTINGS

// battery reading with 2 fresh AA batteries
const int DEFAULT_BATTERY_FULL_ADC_READING = 960;
// batter reading at 2.5 V - ESP will stop working here
const int DEFAULT_BATTERY_EMPTY_ADC_READING = 800;
// sleep time in microseconds
const int DEFAULT_DEEP_SLEEP_MINUTES = 60;
// use led or not - the led is good for debugging, but not for battery life
const bool DEFAULT_USE_LED = true;
// start calibration wizard or not - when checked, a validation wizard will be started
const bool DEFAULT_START_CALIBRATION = false;
// Moisture dry reading @3.0V
const int DEFAULT_MOIST_DRY_READING_AT_3V = 727;
// Moisture wet reading @3.0V
const int DEFAULT_MOIST_WET_READING_AT_3V = 540;

// Pin settings
const int PIN_CLK    = D5;
const int PIN_SENSOR = A0; 
const int PIN_LED    = D7;
const int PIN_SWITCH = D8;
const int PIN_BUTTON = D6;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;

// Samples to read from sensor to calculate average
const int SAMPLE_COUNT = 3;

// if you want to have verbose output, uncomment this:
// #define DEBUG

#endif