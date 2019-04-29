# esp-soil-moisture-sensor

This repository contains code to use the [Homie](https://homieiot.github.io/homie-esp8266/docs/develop-v3/quickstart/what-is-it/) framework and its IoT convention with a soil moisture sensor from [April Brothers](https://wiki.aprbrother.com/en/ESP_Soil_Moisture_Sensor.html). Source and schematic: https://github.com/AprilBrother/esp-soil-moisture-sensor

## How to flash and configure
This code uses [PlatformIO](https://platformio.org/) for easy compilation and uploading. Just open PlatformIO and open the source folder and click on build. PlatformIO will download all necessary libraries and installs them for you.

### Flashing

1. Set the Jumper to the open position
2. Connect programmer
3. Build and upload
4. Power cycle the device

### Configuring

* First, open the Homie [configuration website](http://setup.homie-esp8266.marvinroger.fr/). 
* After flashing, the device will open a wifi access point with a name like `homie-1234567`. Connect to this wifi.
* The configuration website will automatically detect the new homie device. Follow the setup procedure. If you plan to integrate the device into Openhab i suggest to leave the base MQTT topic to the default value `homie`.
* The device will restart and connect to your wifi and immediately start sending to the specified MQTT broker.
* Close the jumper to enable deep sleep
* Power cycle the the device. Otherwise it will stop after the first transmission and not got to deep sleep.

### Debugging

More to come

## Adding the device to openhab2

More to come
