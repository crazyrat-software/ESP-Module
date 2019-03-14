#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "ESP-Config.h"
#include "ESP-Common.h"
#include "ESP-AccessPoint.h"


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
