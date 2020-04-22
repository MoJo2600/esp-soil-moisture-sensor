#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                 // a File variable to temporarily store the received file

const char *ssid = "soilmoisture"; // The name of the Wi-Fi network that will be created
const char *password = "";   // The password required to connect to it, leave blank for an open network

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

// Pin settings
const int PIN_CLK    = D5;
const int PIN_SENSOR = A0; 
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
    yield();
  }
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

// -------------------------------

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("TellMyWifiLoveHer", "2040791920407919");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}


void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
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
int readSensor() {
  int total = 0;
  int rawVal = 0;
  int ret = 0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SENSOR);
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
int getMoisture(int batteryCharge) {
  // Connect Moisture sensor to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, HIGH);
  nonBlockingDelay(200);

  int moisture = 0;
  int moist_raw = readSensor();
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
int getBattery() {
  // Connect Battery to the Pin via on PCB switch
  digitalWrite(PIN_SWITCH, LOW);
  nonBlockingDelay(200);

  int batteryCharge = 0;
  int battery_raw = readSensor();

  return battery_raw;
}

/* 
 * Function: getSendTemperature
 * ----------------------------
 * This function reads the temprature over i2c bus.
 * It then publishes this value via MQTT.
 */
float getTemperature() {
  float temperature = 0.0;
  
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
    int msb = Wire.read();
    int lsb = Wire.read();

    int rawtmp = msb << 8 | lsb;
    int value = rawtmp >> 4;
    temperature = value * 0.0625;
  }
  return temperature;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
    //   if (payload[0] == '#') {            // we get RGB data
    //     uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
    //     int r = ((rgb >> 20) & 0x3FF);                     // 10 bits per color, so R: bits 20-29
    //     int g = ((rgb >> 10) & 0x3FF);                     // G: bits 10-19
    //     int b =          rgb & 0x3FF;                      // B: bits  0-9

    //     analogWrite(LED_RED,   r);                         // write it to the LED output pins
    //     analogWrite(LED_GREEN, g);
    //     analogWrite(LED_BLUE,  b);
    //   } else if (payload[0] == 'R') {                      // the browser sends an R when the rainbow effect is enabled
    //     rainbow = true;
    //   } else if (payload[0] == 'N') {                      // the browser sends an N when the rainbow effect is disabled
    //     rainbow = false;
    //   }
      break;
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}


void setup() {
  Serial.begin(74880);
  delay(10);

  // Prepare pins
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);

  // Set GPIO16 (=D0) pin mode to allow for deep sleep
  // Connect D0 to RST for this to work.
  pinMode(D0, WAKEUP_PULLUP);
  // initialize pin states
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_SWITCH, LOW);
  digitalWrite(PIN_BUTTON, HIGH);

  // Setup I2C library
  Wire.begin();

  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));           // sends instruction byte
  Wire.write(0x60);                 // sends potentiometer value byte
  Wire.endTransmission();           // stop transmitting
  
  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);

  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
}

unsigned long previousMillis = 0;        // will store last time LED was updated

void loop() {

  unsigned long now = millis();

  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server

  int battery_raw = 0;

  char cstr[16];
  char dstr[8];

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 3000)
  {
    previousMillis = currentMillis;
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);

    battery_raw = getBattery();
    doc["battery"] = battery_raw;
    doc["temperature"] = getTemperature();
    doc["moisture"] = getMoisture(battery_raw);

    // webSocket.broadcastTXT(itoa(battery_raw, cstr, 10));
    // webSocket.broadcastTXT(itoa(getTemperature(), cstr, 10));
    // dtostrf(getMoisture(battery_raw), 6, 2, dstr);

    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);

    webSocket.broadcastTXT(output);
    now = millis();
  }
}