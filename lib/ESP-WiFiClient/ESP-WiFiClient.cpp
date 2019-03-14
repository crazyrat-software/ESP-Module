#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP-Config.h"
#include "ESP-Common.h"
#include "ESP-WiFiClient.h"

void initWiFi() {
  Serial.println("[*] Setting up Wireless client");
  cfgAPMAC = WiFi.softAPmacAddress();
  cfgClientMAC = WiFi.macAddress();
  myPassword = cfgPASSWORD;
  myPassword += cfgAPMAC.c_str(); // password longer than 8 chars == WPA2
  myPassword.replace(":", "");
  mySSID = cfgSSID + cfgAPMAC;
  mySSID.replace(":", "-");
  for (int i=0; i < WiFiSSIDsCount; i++) {
    if (DEBUG) { Serial.print("    SSID: "); Serial.println(cfgWiFiSSIDs[i]); }
    wifiMulti.addAP(cfgWiFiSSIDs[i].c_str(), cfgWiFiPasswords[i].c_str());
  }
}

ESP8266WiFiMulti wifiMulti;

void connectWiFi() {
  for (int x = 0; x < 10; x++) {
    wifiMulti.run();
    if (WiFi.status() != WL_CONNECTED) {
      if ( x == 9) {
        Serial.println("[!] No known Wireless networks found!");
        break;
      }
    }
    else {
      WiFiClientConnected = true;
      Serial.println("[-] WiFi Connected");
      Serial.print("    SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("    Wireless Client IP Address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    delay(1000);
  }
}
