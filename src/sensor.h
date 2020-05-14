void nonBlockingDelay(int waitmillis);
void initializeTemperatureSensor(int address);
int readSensor(int sensor_pin);
int getMoisture(int switch_pin, int sensor_pin);
int getBattery(int switch_pin, int sensor_pin);
float getTemperature(int address);
