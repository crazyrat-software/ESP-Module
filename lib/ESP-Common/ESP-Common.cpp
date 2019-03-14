#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <FS.h>
#include "ESP-Config.h"
#include "ESP-Common.h"

bool tickTimer0 = false;
bool tickTimer1 = false;
bool tickTimer2 = false;
Ticker Timer0;
Ticker Timer1;
Ticker Timer2;
char sysSeconds = 0;
char sysMinutes = 0;
uint loopCounter = 0;

String cfgAPMAC = "";
String cfgClientMAC = "";
String mySSID = "";
String myPassword = "";
bool WiFiClientConnected = false;


void timer0Callback() {
  tickTimer0 = true;
}

void timer1Callback() {
  tickTimer1 = true;
  if (sysSeconds >= 59) {
    sysSeconds = 0;
  }
  else {
    sysSeconds++;
  }
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

  size_t size = configFile.size();
  if (size > maxConfigFileSize) {
    if (DEBUG) { Serial.println("[!] Config file size is too large"); }
    return false;
  }
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<maxConfigFileSize> jsonBuffer;
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
  StaticJsonBuffer<maxConfigFileSize> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  // Variable => JSON mapping
  json["Debug"] = DEBUG;
  json["ControlServersRetryCount"] = cfgControlServersRetryCount;
  json["ControlServersRetryMinutes"] = cfgControlServersRetryMinutes;

  JsonArray& arr = json.createNestedArray("ControlServers");
  for (int i = 0; i < ControlServersCount; i++) {
    if (cfgControlServers[i].length() > 0) { arr.add(cfgControlServers[i]); }
    else { arr.add(""); }
  }

  JsonArray& arr2 = json.createNestedArray("WiFiSSID");
  for (int i = 0; i < WiFiSSIDsCount; i++) {
      if (cfgWiFiSSIDs[i].length() > 0) { arr2.add(cfgWiFiSSIDs[i]); }
      else { arr2.add(""); }
  }

  JsonArray& arr3 = json.createNestedArray("WiFiPassword");
  for (int i = 0; i < WiFiSSIDsCount; i++) {
      if (cfgWiFiPasswords[i].length() > 0) { arr3.add(cfgWiFiPasswords[i]); }
      else { arr3.add(""); }
  }

  JsonArray& arr4 = json.createNestedArray("GPIOMode");
  for (int i = 0; i < PinsCount; i++) {
      arr4.add(cfgPinsMode[i]);
  }

  JsonArray& arr5 = json.createNestedArray("GPIOLabels");
  for (int i = 0; i < PinsCount; i++) {
      if (cfgPinsLabels[i].length() > 0) { arr5.add(cfgPinsLabels[i]); }
      else { arr5.add(""); }
  }

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("[!] Failed to open config file for writing");
    return false;
  }
  json.printTo(configFile);
  return true;
}

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

String JSONGetGPIO() {
  tmpStr = "{\"";
  tmpStr += cfgMachine;
  tmpStr += "\": \"";
  tmpStr += cfgAPMAC;
  tmpStr += "\", \"GPIOCount\": ";
  tmpStr += PinsCount;
  tmpStr += ", \"GPIO\": {";
  for (int i = 0; i < PinsCount; i++) {
    tmpStr += "\"GPIO";
    tmpStr += cfgPins[i];
    tmpStr += "\": ";
    if (digitalRead(cfgPins[i]) == HIGH) {
      tmpStr += "1";
    }
    else {
      tmpStr += "0";
    }
    if (i < (PinsCount - 1)) {
      tmpStr += ", ";
    }
  }
  tmpStr += "}, \"A0\": ";
  tmpStr += analogRead(A0);
  tmpStr += "}";
  return tmpStr;
}