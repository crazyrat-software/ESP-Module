#define Enable_AccessPoint 1
#define Enable_WiFiClient 1
#define Enable_WebServer 1
#define Enable_WebClient 1

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>
#include <FS.h>
#include <Esp.h>
#include <ArduinoJson.h>

/*
     Global configuration
*/
#define VERSION "1.3"
#define Update_Server "http://crazyrat.pl/esp/update.php"
bool DEBUG = false;
bool firstRun = false;
const char *cfgMachine = "ESP8266";
const char *cfgSSID = "ESP-";
const char *cfgPASSWORD = "P";
const IPAddress cfgAPIP(192, 168, 128, 1);
const IPAddress cfgAPGateway(192, 168, 128, 1);
const IPAddress cfgAPMask(255, 255, 255, 0);
constexpr int cfgListenPort = 80;

// pins defined to be used on board
const int cfgPins[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10};
String cfgPinsLabels[] = {"D0 (LED2)", "D1", "D2", "D3 (FLASH)", "D4 (LED1)", "D5", "D6", "D7", "D8", "D9 (RX)", "D10 (TX)"};
constexpr int PinsCount = sizeof(cfgPins) / sizeof(cfgPins[0]);
// one of: INPUT=0, INPUT_PULLUP, OUTPUT=1, -1 = don't initialize,
int cfgPinsMode[] = {OUTPUT, OUTPUT, OUTPUT, -1, OUTPUT, INPUT, INPUT, INPUT, INPUT, -1, -1};

constexpr char LED1 = D4;
constexpr char LED2 = D0;
constexpr char ButtonFLASH = D3;
constexpr uint cfgTimer0ms = 1000;       // tick every second
constexpr uint cfgTimer1ms = 10;
constexpr char cfgTimer2s = 60;          // tick every minute
bool tickTimer0 = false;
bool tickTimer1 = false;
bool tickTimer2 = false;
String html = "";
String tmpStr = "";

String cfgControlServers[] = {"http://192.168.1.238/esp/register.php", "http://crazyrat.pl/esp/register.php"};
int cfgControlServersRetryCount = 5;      //disabling after this number of retries
int cfgControlServersRetryMinutes = 15;   //retry disabled servers (minutes)
// -100=disabled; <0=error registration; 0=unregistered; 1=registered
int ControlServersStatus[] = {0, 0};
char ControlServersIndex = 0;             //currently requested ControlServer
char ControlServerStep = 2;               //request once per step (1s)
constexpr char ControlServersCount = sizeof(cfgControlServers) / sizeof(cfgControlServers[0]);

String cfgWiFiSSIDs[5] = {""};
String cfgWiFiPasswords[5] = {""};
constexpr char WiFiSSIDsCount = 5;


/*
 *   Core functions
 */
void GPIOInit(bool prn = false) {
  Serial.println("[*] Setting up GPIO");
  for (int i = 0; i < PinsCount; i++) {
    // ButtonFLASH nie moze pracowac jako OUTPUT aby moc wykorzystac przycisk do wlaczenia DEBUG mode
    if (cfgPins[i] != ButtonFLASH) {
      if (prn) {
        Serial.print("    ");
        tmpStr = cfgPinsLabels[i];
        Serial.print(tmpStr);
        for (int i = 0; i < (16 - tmpStr.length()); i++ ) {
          Serial.print(" ");
        }
        Serial.print(": ");
      }
      if (cfgPinsMode[i] < 0) {
        if (prn) {
          Serial.println(ModeToString(cfgPinsMode[i]));
        }
      }
      else {
        pinMode(cfgPins[i], cfgPinsMode[i]);
        if (prn) {
          Serial.println(ModeToString(cfgPinsMode[i]));
        }
      }
    }
    else if (prn) {
      Serial.print("    ");
      tmpStr = cfgPinsLabels[i];
      Serial.print(tmpStr);
      for (int i = 0; i < (16 - tmpStr.length()); i++ ) {
        Serial.print(" ");
      }
      Serial.print(": ");
      Serial.println("INPUT (not configurable)");
    }
  }
  pinMode(ButtonFLASH, INPUT);
}



/*
     Wireless functions
*/
#if Enable_WiFiClient == 1 || Enable_AccessPoint == 1
String cfgAPMAC = "";
String cfgClientMAC = "";
String mySSID = "";
String myPassword = "";
bool WiFiClientConnected = false;

void initWiFi() {
  cfgAPMAC = WiFi.softAPmacAddress();
  cfgClientMAC = WiFi.macAddress();
  myPassword = cfgPASSWORD;
  myPassword += cfgAPMAC.c_str(); // password longer than 8 chars == WPA2
  myPassword.replace(":", "");
  mySSID = cfgSSID + cfgAPMAC;
  mySSID.replace(":", "-");
}
#endif

#if Enable_WiFiClient == 1
ESP8266WiFiMulti wifiMulti;

void connectWiFi() {
  Serial.println("[*] Setting up Wireless client");
  for (int i=0; i < WiFiSSIDsCount; i++) {
    if (DEBUG) { Serial.print("    SSID: "); Serial.println(cfgWiFiSSIDs[i]); }
    wifiMulti.addAP(cfgWiFiSSIDs[i].c_str(), cfgWiFiPasswords[i].c_str());
  }
  
  for (int x = 0; x < 10; x++) {
    wifiMulti.run();
    delay(1000);
    if (WiFi.status() != WL_CONNECTED) {
      if ( x == 9) {
        Serial.println("[!] No known Wireless networks found!");
        break;
      }
    } else {
      WiFiClientConnected = true;
      Serial.println("[-] WiFi Connected");
      Serial.print("    SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("    Wireless Client IP Address: ");
      Serial.println(WiFi.localIP());
      break;
    }
  }
}
#endif

#if Enable_AccessPoint == 1
void startAccessPoint() {
  Serial.println("[*] Setting up Access Point");
  WiFi.softAPConfig(cfgAPIP, cfgAPGateway, cfgAPMask);
  WiFi.softAP(mySSID.c_str(), myPassword.c_str());
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("[-] SSID: ");
  Serial.println(mySSID);
  Serial.print("[-] Access Point IP Address: ");
  Serial.println(myIP);
}
#endif

/*
     Web Server functions
*/
#if Enable_WebServer == 1
ESP8266WebServer server(cfgListenPort);
File fsUploadFile;

int getPinFromURL(String url, String prefix = "/setGPIO") {
  // return number from uri "prefix#number#/"
  String tmpStr = url;
  if (tmpStr.startsWith(prefix)) {
    tmpStr.remove(0, prefix.length());
    tmpStr.remove(tmpStr.indexOf('/'));
    for (int i = 0; i < PinsCount; i++) {
      if (cfgPins[i] == tmpStr.toInt()) {
        return tmpStr.toInt();
      }
    }
  }
  return -1;
}

int getPinValueFromURL(String url, String prefix = "/setGPIO") {
  // return value from uri "prefix#number#/#value#"
  String tmpStr = url;
  if (tmpStr.startsWith(prefix)) {
    tmpStr.remove(0, prefix.length());
    if (tmpStr.indexOf('/') > 0) {
      tmpStr.remove(0, tmpStr.indexOf('/') + 1);
      tmpStr.toLowerCase();
      if (tmpStr == "on") {
        return 1;
      }
      if (tmpStr == "off") {
        return 0;
      }
    }
  }
  return -1;
}

int getPinModeFromURL(String url, String prefix = "/setModeGPIO") {
  // return value from uri "prefix#number#/#value#"
  String tmpStr = url;
  if (tmpStr.startsWith(prefix)) {
    tmpStr.remove(0, prefix.length());
    if (tmpStr.indexOf('/') > 0) {
      tmpStr.remove(0, tmpStr.indexOf('/') + 1);
      tmpStr.toLowerCase();
      if (tmpStr == "output") {
        return 1;
      }
      if (tmpStr == "input") {
        return 0;
      }
    }
  }
  return -1;
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

String header() {
  return "<!DOCTYPE html><head><title>IoT</title></head><link href=\"styles.css\" rel=\"stylesheet\"><body>";
}

String footer() {
  return "<br><hr><small>IoT: <strong>" + cfgAPMAC + "</strong></small>";
}

void startWebserver() {
  Serial.println(F("[*] Setting up WebServer"));

  // list of all handled requests
  server.on("/esp", handleRoot);
  server.on("/showStatus", handleShowStatus);
  server.on("/showState", handleShowState);
  server.on("/getGPIO", handleGetGPIO);
  server.on("/getMode", handleGetMode);
  server.on("/getModeStr", handleGetModeStr);
  server.on("/configReload", handleConfigReload);
  server.on("/sysRestart", handleSysRestart);

  // define all /setGPIO#number#/on
  for (int i = 0; i < PinsCount; i++) {
    tmpStr = "/setGPIO";
    tmpStr += cfgPins[i];
    tmpStr += "/on";
    if (DEBUG) {
      Serial.print(F("    Binding to: "));
      Serial.println(tmpStr);
    }
    server.on(tmpStr.c_str(), handleSetGPIO);
  }
  // define all /setGPIO#number#/off
  for (int i = 0; i < PinsCount; i++) {
    tmpStr = "/setGPIO";
    tmpStr += cfgPins[i];
    tmpStr += "/off";
    if (DEBUG) {
      Serial.print(F("    Binding to: "));
      Serial.println(tmpStr);
    }
    server.on(tmpStr.c_str(), handleSetGPIO);
  }

  // define all /setModeGPIO#number#/0
  for (int i = 0; i < PinsCount; i++) {
    tmpStr = "/setModeGPIO";
    tmpStr += cfgPins[i];
    tmpStr += "/0";
    if (DEBUG) {
      Serial.print(F("    Binding to: "));
      Serial.println(tmpStr);
    }
    server.on(tmpStr.c_str(), handleSetMode);
  }
  // define all /setModeGPIO#number#/1
  for (int i = 0; i < PinsCount; i++) {
    tmpStr = "/setModeGPIO";
    tmpStr += cfgPins[i];
    tmpStr += "/1";
    if (DEBUG) {
      Serial.print(F("    Binding to: "));
      Serial.println(tmpStr);
    }
    server.on(tmpStr.c_str(), handleSetMode);
  }

  // file upload application
  server.on("/list", handleFileList);
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  // try to serve index.htm if not then try to serve handleRoot()
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  server.begin();
  //server.setNoDelay(true);
  Serial.print(F("[-] Listening at: "));
  Serial.println(cfgListenPort);
}

void handleRoot() {
  html = "<h1>IoT</h1><table>"
         "<tr><th>Action</th><th>Description</th></tr>"
         "<tr><td colspan=2>HTML Methods</td></tr>"
         "<tr><td><a href=\"/esp\">esp</a></td><td>Show this page.</td></tr>"
         "<tr><td><a href=\"/showStatus\">showStatus</a></td><td>Show device status.</td></tr>"
         "<tr><td><a href=\"/showState\">showState</a></td><td>Show table with states of all GPIOs.</td></tr>"
         "<tr><td colspan=2>JSON Methods</td></tr>"
         "<tr><td><a href=\"/getGPIO\">getGPIO</a></td><td>Get JSON value of all GPIOs.</td></tr>"
         "<tr><td><a href=\"/getMode\">getMode</a></td><td>Get JSON mode (integer value) of all GPIOs.</td></tr>"
         "<tr><td><a href=\"/getModeStr\">getModeStr</a></td><td>Get JSON mode (string representation) of all GPIOs.</td></tr>";
         "<tr><td><a href=\"/getModeStr\">getModeStr</a></td><td>Get JSON mode (string representation) of all GPIOs.</td></tr>";
         "<tr><td><a href=\"/configReload\">configReload</a></td><td>Load config from config file.</td></tr>";
         "<tr><td><a href=\"/sysRestart\">sysRestart</a></td><td>System restart</td></tr>";

  // setGPIO
  tmpStr = "";
  for (int i = 0; i < PinsCount; i++) {
    if (cfgPinsMode[i] == OUTPUT) {
      tmpStr += "<tr><td>[";
      tmpStr += "<a href='/setGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/off'>OFF</a>]";
      tmpStr += "<a href='/setGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/on'>ON</a>]";
      tmpStr += "</td><td>";
      tmpStr += cfgPinsLabels[i];
      tmpStr += "</td></tr>";
    }
  }
  html += tmpStr;

  // setModeGPIO
  tmpStr = "";
  for (int i = 0; i < PinsCount; i++) {
    if (cfgPinsMode[i] != -1) {
      tmpStr += "<tr><td>[";
      tmpStr += "<a href='/setModeGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/1'>OUTPUT</a>]";
      tmpStr += "<a href='/setModeGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/0'>INPUT</a>]";
      tmpStr += "</td><td>";
      tmpStr += cfgPinsLabels[i];
      tmpStr += "</td></tr>";
    }
  }
  html += tmpStr;

  html += "<tr><td colspan=2>File System Methods</td></tr>";
  html += "<tr><td><a href=\"/edit\">edit</a></td><td>Web browser for local filesystem.</td></tr>"
          "</table><br><br>";
  server.send(200, "text/html", header() + html + footer());
}

bool handleFileRead(String path) {
  if (DEBUG) {
    Serial.println("handleFileRead: " + path);
  }
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  else {
    handleRoot();
  }
  //return false;
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  if (DEBUG == true) {
    Serial.println("handleFileList: " + path);
  }
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleShowStatus() {
  html = "<h1>IoT</h1>";
  html += "<table>";
  html += "<tr><th>Running on</th><td>";
  html += cfgMachine;
  html += "</td></tr><tr><th>Running at</th><td>";
  html += system_get_cpu_freq();
  html += "MHz";
  html += "</td></tr><tr><th>Memory</th><td>";
  html += system_get_free_heap_size();
  html += "</td></tr><tr><th>Access Point SSID</th><td>";
  html += mySSID;
  html += "</td></tr><tr><th>Access Point MAC</th><td>";
  html += cfgAPMAC;
  html += "</td></tr><tr><th>Access Point IP</th><td>";
  html += WiFi.softAPIP().toString();
  html += "</td></tr><tr><th>Connected clients number</th><td>";
  html += wifi_softap_get_station_num();
  html += "</td></tr><tr><th>Wireless Client SSID</th><td>";
  html += WiFi.SSID();
  html += "</td></tr><tr><th>Wireless Client MAC</th><td>";
  html += cfgClientMAC;
  html += "</td></tr><tr><th>Wireless Client IP</th><td>";
  html += WiFi.localIP().toString();
  html += "</td></tr><tr><th>Registered to Control Servers</th><td>";
  for (int i = 0; i < ControlServersCount; i++) {
    if (ControlServersStatus[i] == 1) {
      html += cfgControlServers[i];
      html += "<br>";
    }
  }
  html += "</td></tr></table>";
  server.send(200, "text/html", header() + html + footer());
}

void handleShowState() {
  tmpStr = "";
  html = "<h1>IoT</h1>";
  html += "<table><tr><th>GPIO</th><th>Mode</th><th>Value</th></tr>";
  int val = 0;
  for (int i = 0; i < PinsCount; i++) {
    if (digitalRead(cfgPins[i]) == HIGH) {
      val = 1;
    }
    else {
      val = 0;
    }
    tmpStr = val;
    html += "<tr><td>";
    html += cfgPinsLabels[i];
    html += "</td><td>";
    html += ModeToString(cfgPinsMode[i]);
    html += "</td><td>" + tmpStr + "</td></tr>";
  }
  html += "<tr><td>AO</td><td>Analog INPUT</td><td>";
  html += analogRead(A0);
  html += "</td></tr></table>";
  server.send(200, "text/html", header() + html + footer());
}

void handleConfigReload() {
  if (JSONLoad()) {
    GPIOInit(true);
    html = "{\"";
    html += cfgMachine;
    html += "\": \"";
    html += cfgAPMAC;
    html += "\", \"Result\": ";
    html += 0;
    html += "}";
    server.send(200, "text/html", html);
  }
  else {
    html = "{\"";
    html += cfgMachine;
    html += "\": \"";
    html += cfgAPMAC;
    html += "\", \"Result\": ";
    html += -1;
    html += "}";
    server.send(200, "text/html", html);  
  }
}

void handleSysRestart() {
  ESP.restart();
}

void handleGetGPIO() {
  html = "{\"";
  html += cfgMachine;
  html += "\": \"";
  html += cfgAPMAC;
  html += "\", \"GPIOCount\": ";
  html += PinsCount;
  html += ", \"GPIO\": {";
  int val = 0;
  for (int i = 0; i < PinsCount; i++) {
    html += "\"GPIO";
    html += cfgPins[i];
    html += "\": ";
    if (digitalRead(cfgPins[i]) == HIGH) {
      html += "1";
    }
    else {
      html += "0";
    }
    if (i < (PinsCount - 1)) {
      html += ", ";
    }
  }
  html += "}, \"A0\": ";
  html += analogRead(A0);
  html += "}";
  server.send(200, "text/html", html);
}

void handleSetGPIO() {
  Serial.println(server.uri());
  int pin = getPinFromURL(server.uri());
  int val = getPinValueFromURL(server.uri());

  if (pin >= 0 and val >= 0) {
    digitalWrite(pin, val);
  }
  html = "{\"";
  html += cfgMachine;
  html += "\": \"";
  html += cfgAPMAC;
  html += "\", \"Result \": ";
  html += val;
  html += "}";
  server.send(200, "text/html", html);
}

void handleGetMode() {
  html = "{\"";
  html += cfgMachine;
  html += "\": \"";
  html += cfgAPMAC;
  html += "\", \"GPIOCount\": ";
  html += PinsCount;
  html += ", \"GPIO\": {";
  int val = 0;
  for (int i = 0; i < PinsCount; i++) {
    html += "\"GPIO";
    html += cfgPins[i];
    html += "\": ";
    html += cfgPinsMode[i];
    if (i < (PinsCount - 1)) {
      html += ", ";
    }
  }
  html += "}, \"A0\": ";
  html += analogRead(A0);
  html += "}";
  server.send(200, "text/html", html);
}

void handleGetModeStr() {
  html = "{\"";
  html += cfgMachine;
  html += "\": \"";
  html += cfgAPMAC;
  html += "\", \"GPIOCount\": ";
  html += PinsCount;
  html += ", \"GPIO\": {";
  int val = 0;
  for (int i = 0; i < PinsCount; i++) {
    html += "\"GPIO";
    html += cfgPins[i];
    html += "\": \"";
    if (cfgPins[i] == ButtonFLASH) {
      html += "INPUT (not configurable)";
    }
    else {
      html += ModeToString(cfgPinsMode[i]);
    }
    html += "\"";
    if (i < (PinsCount - 1)) {
      html += ", ";
    }
  }
  html += "}, \"A0\": ";
  html += analogRead(A0);
  html += "}";
  server.send(200, "text/html", html);
}

void handleSetMode() {
  Serial.println(server.uri());
  int pin = getPinFromURL(server.uri(), "/setModeGPIO");
  int val = getPinModeFromURL(server.uri(), "/setModeGPIO");

  if (pin >= 0 and val >= 0) {
    pinMode(pin, val);
  }
  html = "{\"";
  html += cfgMachine;
  html += "\": \"";
  html += cfgAPMAC;
  html += "\", \"Result\": ";
  html += val;
  html += "}";
  server.send(200, "text/html", html);

}
#endif

/*
     Web Client functions
*/
#if Enable_WebClient == 1
HTTPClient WebClient;

int requestURL(String url) {
  int code;
  WebClient.begin(url);
  WebClient.setReuse(true);
  code = WebClient.GET();
  WebClient.end();
  return code;
}

void ControlServerRegister() {
  int httpcode;
  if (ControlServersIndex >= ControlServersCount) {
    ControlServersIndex = 0;
  }
  int idx = ControlServersIndex;
  for (int i = 0; i < ControlServersCount; i++) {
    if (ControlServersStatus[idx] <= 0 && ControlServersStatus[idx] != -100) {
      WebClient.begin(cfgControlServers[idx]);
      WebClient.setReuse(false);
      tmpStr = cfgMachine;
      tmpStr += "_";
      tmpStr += mySSID;
      WebClient.addHeader("ESP", tmpStr);
      httpcode = WebClient.GET();
      tmpStr =  WebClient.getString();
      if ((httpcode == 200) && (tmpStr == "{\"Result\": 0}")) {
        Serial.print(F("[-] Control Server Registered: "));
        Serial.println(cfgControlServers[idx]);
        if (DEBUG) {
          Serial.println(tmpStr);
        }
        ControlServersStatus[idx] = 1;
      }
      else {
        Serial.print(F("[-] Control Server Registration failed: "));
        Serial.print(cfgControlServers[idx]);
        ControlServersStatus[idx]--;
        Serial.print(F(" (Try: "));
        Serial.print(-ControlServersStatus[idx]);
        Serial.println(F(")"));
        if (ControlServersStatus[idx] <= -cfgControlServersRetryCount) {
          Serial.print(F("[!] Disabling Control Server: "));
          Serial.println(cfgControlServers[idx]);
          ControlServersStatus[idx] = -100;
        }
      }
      WebClient.end();
      break;
    }
    idx++;
    if (idx >= ControlServersCount) {
      idx = 0;
    }
  }
  ControlServersIndex++;
}
#endif

/*
     Main program setup and loop
*/
Ticker Timer0;
Ticker Timer1;
Ticker Timer2;
uint loopCounter = 0;
char sysSeconds = 0;
char sysMinutes = 0;
char httpcode;
char iteration;

void timer0Callback() {
  tickTimer0 = true;
  if (sysSeconds == 59) {
    sysSeconds = 0;
  }
  else {
    sysSeconds++;
  }
}

void timer1Callback() {
  tickTimer1 = true;
}

void timer2Callback() {
  tickTimer2 = true;
  if (sysMinutes == 59) {
    sysMinutes = 0;
  }
  else {
    sysMinutes++;
  }
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

const char* ModeToString(char m) {
  const char* modes[] = {"unchanged", "INPUT", "OUTPUT"};
  if (m == INPUT) {
    return modes[1];
  }
  else if (m == OUTPUT) {
    return modes[2];
  }
  else return modes[0];
}

const char* ResetReasonToString(char r) {
  const char* reasons[] = {"PowerON", "Hardware Watchdog Reset", "Exception Reset", "Software Watchdog Reset", "Software Restart", "Wake Up", "External Restart"};
  if (r >= 0) {
    return reasons[r];
  }
  else return reasons[2];
}

void checkUpdates() {
  Serial.println(F("[*] Checking for updates..."));
  //t_httpUpdate_return  ret = ESPhttpUpdate.update(Update_Server, VERSION, "29 AE 7F EC 3F 5D A1 37 A8 F3 D6 13 4E 9E 06 5D 6A 78 15 51");
  t_httpUpdate_return  ret = ESPhttpUpdate.update(Update_Server, VERSION);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[!] Update failed! Error %d: %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      Serial.println();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[-] No updates."));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("[-] Update available."));
      break;
  }
}

bool assign(const JsonObject& json, const char* keyStr, int &value) {
  if (json.containsKey(keyStr)) {
    value = json[keyStr].as<int>();
    return true;
  }
  return false;
}

bool assign(const JsonObject& json, const char* keyStr, bool &value) {
  if (json.containsKey(keyStr)) {
    value = json[keyStr].as<bool>();
    return true;
  }
  return false;
}

// assign as array of char* with index
bool assign(const JsonObject& json, const char* keyStr, String &value, int idx) {
  if (json.containsKey(keyStr)) {
    JsonArray& arr =  json[keyStr];
    value = arr[idx].as<const char*>();
    return true;
  }
  return false;
}

// assign as array of int with index
bool assign(const JsonObject& json, const char* keyStr, int &value, int idx) {
  if (json.containsKey(keyStr)) {
    JsonArray& arr =  json[keyStr];
    value = arr[idx].as<int>();
    return true;
  }
  return false;
}

bool JSONLoad() {
  Serial.println("[-] Loading config file");
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    if (DEBUG) { Serial.println("[!] Failed to open config file"); }
    return false;
  }

  constexpr int maxCfgSize = 1024;
  size_t size = configFile.size();
  if (size > maxCfgSize) {
    if (DEBUG) { Serial.println("[!] Config file size is too large"); }
    return false;
  }
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<maxCfgSize> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    if (DEBUG) { Serial.println("[!] Failed to parse config file"); }
    return false;
  }
  
  // JSON => variable mapping
  if (assign(json, "Debug" , DEBUG) && DEBUG) { Serial.println(F("    Reading value: DEBUG")); }
  if (assign(json, "ControlServersRetryCount" , cfgControlServersRetryCount) && DEBUG) { Serial.println(F("    Reading value: ControlServersRetryCount")); }
  if (assign(json, "ControlServersRetryMinutes" , cfgControlServersRetryMinutes) && DEBUG) { Serial.println(F("    Reading value: ControlServersRetryMinutes")); }
  for (int i=0; i<ControlServersCount; i++) { if (assign(json, "ControlServers" , cfgControlServers[i], i) && DEBUG) { Serial.println(F("    Reading array: ControlServers[]")); } }
  for (int i=0; i<WiFiSSIDsCount; i++) { if (assign(json, "WiFiSSID" , cfgWiFiSSIDs[i], i) && DEBUG) { Serial.println(F("    Reading array: WiFiSSID[]")); } }
  for (int i=0; i<WiFiSSIDsCount; i++) { if (assign(json, "WiFiPassword" , cfgWiFiPasswords[i], i) && DEBUG) { Serial.println(F("    Reading array: WiFiPassword[]")); } }
  for (int i=0; i<PinsCount; i++) { if (assign(json, "GPIOMode" , cfgPinsMode[i], i) && DEBUG) { Serial.println(F("    Reading array: GPIOMode[]")); } }
  for (int i=0; i<PinsCount; i++) { if (assign(json, "GPIOLabels" , cfgPinsLabels[i], i) && DEBUG) { Serial.println(F("    Reading array: GPIOLabels[]")); } }
  
  return true;
}

bool JSONSave() {
  Serial.println("[-] Saving config file");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  // Variable to JSON mapping
  //json["serverName"] = "api.example.com";
  //json["accessToken"] = "128du9as8du12eoue8da98h123ueh9h98";

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("[!] Failed to open config file for writing");
    return false;
  }
  json.printTo(configFile);
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println(); Serial.println(); Serial.println();
  system_update_cpu_freq(160);
  delay(100);
  Serial.print(F("[*] Kernel version: "));
  Serial.println(VERSION);
  Serial.print(F("[*] Init running at: "));
  Serial.print(system_get_cpu_freq());
  Serial.print(F("MHz, Reset Reason: "));
  Serial.println(ESP.getResetReason());

  // wait for FLASH button to be pressed
  // this feature could be disabled in PROD Firmware
  for (int i = 0; i < 500; i++) {
    if (!digitalRead(ButtonFLASH))
    {
      DEBUG = true;
      Serial.setDebugOutput(DEBUG);
      Serial.println(F("[*] Debug flag enabled"));
      break;
    }
    delay(1);
  }

  Serial.println(F("[*] Mounting filesystem"));
  //SPIFFS.format();
  if (SPIFFS.begin()) {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      if (DEBUG) {
        Serial.printf("    File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      }
    }
    if (!JSONLoad()) {
      Serial.println("[!] Failed to load config");
    }
  }

  GPIOInit(true);

#if Enable_WifiClient == 1 || Enable_AccessPoint == 1
  initWiFi();
#endif
#if Enable_WiFiClient == 1
  connectWiFi();
#endif
#if Enable_AccessPoint == 1
  startAccessPoint();
#endif
#if Enable_WebServer == 1
  startWebserver();
#endif

#if Enable_WebClient == 1
  Serial.println(F("[-] Control Servers:"));
  for (int i = 0; i < ControlServersCount; i++) {
    Serial.print(F("    "));
    Serial.println(cfgControlServers[i]);
  }
#endif
}

void loop() {
  if (firstRun == false) {
    firstRun = true;
    checkUpdates();
    Serial.println(F("[*] Exec."));
    Timer0.attach_ms(cfgTimer0ms, timer0Callback);
    Timer1.attach_ms(cfgTimer1ms, timer1Callback);
    Timer2.attach(cfgTimer2s, timer2Callback);
  }

  // Fast timer
  if (tickTimer1 == true) {
#if Enable_WiFiClient == 1
    if (WiFiClientConnected) {
      digitalWrite(LED1, HIGH);
    }
#endif
    digitalWrite(LED2, HIGH);
    Timer1.detach();
    tickTimer1 = false;
  }

  // Slow timer (1s)
  if (tickTimer0 == true) {
    if (DEBUG) {
      Serial.print("tick Timer0 loopCounter=");
      Serial.println(loopCounter);
    }
#if Enable_WiFiClient == 1
    if (WiFiClientConnected) {
      digitalWrite(LED1, LOW);
    }
#endif
    digitalWrite(LED2, LOW);
    Timer1.attach_ms(cfgTimer1ms, timer1Callback);

#if Enable_WebClient == 1
    iteration++;
    if (iteration == ControlServerStep) {
      ControlServerRegister();
      iteration = 0;
    }
#endif

    loopCounter = 0;
    tickTimer0 = false;
  }

  // Very slow timer (60s)
  if (tickTimer2 == true) {
    if (DEBUG) {
      Serial.print("tick Timer2 loopCounter=");
      Serial.println(loopCounter);
    }
#if Enable_WebClient == 1
    Serial.println("[-] Unregistering Control Servers");
    //for (int i = 0; i < cfgControlServersCount; i++) { if (ControlServersStatus[i] == 1) { ControlServersStatus[i] = 0; }}
    for (auto& j : ControlServersStatus) {
      if (j == 1) {
        j = 0;
      }
    }

    if ((sysMinutes % cfgControlServersRetryMinutes) == 0) {
      Serial.println("[-] Enabling Disabled Control Servers");
      //for (int i = 0; i < cfgControlServersCount; i++) { ControlServersStatus[i] = 0; }
      for (auto& j : ControlServersStatus) {
        j = 0;
      }
    }
#endif

    tickTimer2 = false;
  }

#if Enable_WebServer == 1
  server.handleClient();
#endif

  loopCounter++;
  delay(0);
}
