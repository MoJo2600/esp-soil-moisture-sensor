#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include <constants.h>
#include <sensor.h>

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

const char* mdnsName = "soilsensor"; // Domain name for the mDNS responder

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

  // get wifi config from homie
  File file = SPIFFS.open("/homie/config.json", "r");                    // Open the file

  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  const char* ssid = doc["wifi"]["ssid"];
  const char* password = doc["wifi"]["password"];

  file.close();                                          // Close the file again

  WiFi.begin(ssid, password);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);

      StaticJsonDocument<512> received;
      // Deserialize the JSON document
      DeserializationError error = deserializeJson(received, payload);
      if (error) {
        Serial.println(F("Failed to read payload"));
        return;
      }

      // get wifi config from homie
      File file = SPIFFS.open("/homie/config.json", "r");               // Open the file
      DynamicJsonDocument doc(1024);
      // Deserialize the JSON document
      error = deserializeJson(doc, file);
      if (error) {
        Serial.println(F("Failed to read file, using default configuration"));
        return;
      }

      doc["settings"]["dryReadingAt3V"] = received["dry"];
      doc["settings"]["wetReadingAt3V"] = received["wet"];
      doc["settings"]["batteryFull"] = received["battery"];
      doc["settings"]["startCalibration"] = false;

      file.close();                                                     // Close the file again

      file = SPIFFS.open("/homie/config.json", "w");                    // Open the file
      serializeJson(doc, file);
      file.close();

      ESP.restart();

      break;
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  // start the multicast domain name server
  if (!MDNS.begin(mdnsName)) {
    Serial.print("Could not start MDNS service!");
  }
  else
  {
    Serial.print("mDNS responder started: http://");
    Serial.print(mdnsName);
    Serial.println(".local");
  }
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}


void setup_wizard() {
  delay(10);

  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
}

unsigned long previousMillis = 0;        // will store last time LED was updated
SensorReading batteryReading;
SensorReading moistureReading;

void wizard_loop() {

  unsigned long now = millis();

  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server


  unsigned long currentMillis = millis();

  if (webSocket.connectedClients() > 0) {
    if (currentMillis - previousMillis >= 3000)
    {
      digitalWrite(PIN_LED, LOW);
      previousMillis = currentMillis;
      // creat JSON message for Socket.IO (event)
      DynamicJsonDocument doc(1024);

      doc["temperature"] = getTemperature();

      batteryReading = getBattery(DEFAULT_BATTERY_EMPTY_ADC_READING, DEFAULT_BATTERY_FULL_ADC_READING);
      doc["battery_raw"] = batteryReading.raw;
      doc["battery"] = batteryReading.adjusted;
      doc["battery_percent"] = batteryReading.percent;

      moistureReading = getMoisture(batteryReading.adjusted, DEFAULT_MOIST_DRY_READING_AT_3V, DEFAULT_MOIST_WET_READING_AT_3V, DEFAULT_BATTERY_FULL_ADC_READING);
      doc["moisture_raw"] = moistureReading.raw;
      doc["moisture"] = moistureReading.adjusted;
      doc["moisture_percent"] = moistureReading.percent;

      // JSON to String (serializion)
      String output;
      serializeJson(doc, output);

      webSocket.broadcastTXT(output);
      now = millis();
      digitalWrite(PIN_LED, HIGH);
    }
  }
}