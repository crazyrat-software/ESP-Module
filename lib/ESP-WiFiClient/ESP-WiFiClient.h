#ifndef ESP_WIFICLIENT_H
#define ESP_WIFICLIENT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

extern ESP8266WiFiMulti wifiMulti;

extern void initWiFi();
extern void connectWiFi();
#endif
