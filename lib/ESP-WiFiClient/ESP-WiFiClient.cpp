#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP-Config.h"
#include "ESP-Common.h"
#include "ESP-WiFiClient.h"

void initWiFi() {
  cfgAPMAC = WiFi.softAPmacAddress();
  cfgClientMAC = WiFi.macAddress();
  myPassword = cfgPASSWORD;
  myPassword += cfgAPMAC.c_str(); // password longer than 8 chars == WPA2
  myPassword.replace(":", "");
  mySSID = cfgSSID + cfgAPMAC;
  mySSID.replace(":", "-");
}

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
