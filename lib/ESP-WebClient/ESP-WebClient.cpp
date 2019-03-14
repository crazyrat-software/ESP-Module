#include <ESP8266HTTPClient.h>
#include "ESP-Common.h"
#include "ESP-Config.h"
#include "ESP-WebClient.h"

HTTPClient WebClient;
char httpcode;

int requestGET(String url) {
  int code;
  WebClient.begin(url);
  WebClient.setReuse(true);
  code = WebClient.GET();
  WebClient.end();
  return code;
}

int requestPOST(String url, String data){
  int code;
  WebClient.begin(url);
  WebClient.addHeader("Content-Type", "text/plain");
  code = WebClient.POST(data);
  WebClient.end();
  return code;
}

void ControlServerRegister() {
  //int httpcode;
  if (ControlServersIndex >= ControlServersCount) {
    ControlServersIndex = 0;
  }
  int idx = ControlServersIndex;
  for (int i = 0; i < ControlServersCount; i++) {
    if (ControlServersStatus[idx] <= 0 && ControlServersStatus[idx] != -100) {
      tmpStr = cfgControlServers[idx];
      tmpStr += "/esp/register.php";
      WebClient.begin(tmpStr);
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


void ControlServerPushData() {
  for (int i = 0; i < ControlServersCount; i++) {
      if (ControlServersStatus[i] == 1) {
        tmpStr = cfgControlServers[i];
        tmpStr += "/esp/esp_getGPIO.php?action=push";
        httpcode = requestPOST(tmpStr, JSONGetGPIO());
        if (DEBUG) {
          Serial.print(F("[-] JSONGetGPIO: "));
          Serial.println(tmpStr);
        }
      }
  }
}