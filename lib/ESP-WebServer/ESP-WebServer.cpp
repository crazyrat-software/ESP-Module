#include <Arduino.h>
#include <FS.h>

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif
#include <Esp.h>

#include <ESP8266WebServer.h>
#include "ESP-Config.h"
#include "ESP-Common.h"
#include "ESP-WebServer.h"

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
  return "<br><hr><small><strong>IoT: </strong>" + cfgAPMAC + ", <strong>Kernel version: </strong>" + VERSION + "</small>";
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
  server.on("/configLoad", handleConfigLoad);
  server.on("/configSave", handleConfigSave);
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
    tmpStr += "/input";
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
    tmpStr += "/output";
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
         "<tr><td><a href=\"/getModeStr\">getModeStr</a></td><td>Get JSON mode (string representation) of all GPIOs.</td></tr>"
         "<tr><td><a href=\"/configLoad\">configLoad</a></td><td>Load config from config file.</td></tr>"
         "<tr><td><a href=\"/configSave\">configSave</a></td><td>Save config to config file.</td></tr>"
         "<tr><td><a href=\"/sysRestart\">sysRestart</a></td><td>System restart</td></tr>";

  // setGPIO
  tmpStr = "<tr><td colspan=2>Set GPIO Value</td></tr>";
  for (int i = 0; i < PinsCount; i++) {
    if (cfgPinsMode[i] == OUTPUT) {
      tmpStr += "<tr><td>[ ";
      tmpStr += "<a href='/setGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/off'>OFF</a> ][ ";
      tmpStr += "<a href='/setGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/on'>ON</a> ]";
      tmpStr += "</td><td>";
      tmpStr += cfgPinsLabels[i];
      tmpStr += "</td></tr>";
    }
  }
  html += tmpStr;

  // setModeGPIO
  tmpStr = "<tr><td colspan=2>Set GPIO Mode</td></tr>";
  for (int i = 0; i < PinsCount; i++) {
    if (cfgPinsMode[i] != -1) {
      tmpStr += "<tr><td>[ ";
      tmpStr += "<a href='/setModeGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/output'>OUTPUT</a> ][ ";
      tmpStr += "<a href='/setModeGPIO";
      tmpStr += cfgPins[i];
      tmpStr += "/input'>INPUT</a> ]";
      tmpStr += "</td><td>";
      tmpStr += cfgPinsLabels[i];
      tmpStr += "</td></tr>";
    }
    else {
      tmpStr += "<tr><td>[ restricted ]</td><td>";
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
  html += "<tr><th>Kernel version</th><td>";
  html += VERSION;
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
  html += "<table><tr><th>GPIO</th><th>Label</th><th>Mode</th><th>Value</th></tr>";
  int val = 0;
  for (int i = 0; i < PinsCount; i++) {
    if (digitalRead(cfgPins[i]) == HIGH) {
      val = 1;
    }
    else {
      val = 0;
    }
    tmpStr = val;
    html += "<tr><td>GPIO";
    html += cfgPins[i];
    html += "</td><td>";
    html += cfgPinsLabels[i];
    html += "</td><td>";
    html += ModeToString(cfgPinsMode[i]);
    html += "</td><td>" + tmpStr + "</td></tr>";
  }
  html += "<tr><td>AO</td><td>Analog</td><td>INPUT</td><td>";
  html += analogRead(A0);
  html += "</td></tr></table>";
  server.send(200, "text/html", header() + html + footer());
}

void handleConfigLoad() {
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

void handleConfigSave() {
  if (JSONSave()) {
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
  int pin = getPinFromURL(server.uri(), "/setModeGPIO");
  int val = getPinModeFromURL(server.uri(), "/setModeGPIO");

  if (pin >= 0 and val >= 0) {
    pinMode(pin, val);
    for (int i = 0; i < PinsCount; i++) {
      if (cfgPins[i] == pin) { cfgPinsMode[i] = val; }
    }
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
