#define Enable_AccessPoint 1
#define Enable_WiFiClient 1
#define Enable_WebServer 1
#define Enable_WebClient 1


#include <Arduino.h>
#include <FS.h>
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

#include <ESP-AccessPoint.h>
#include <ESP-WiFiClient.h>
#include <ESP-WebServer.h>
#include <ESP-WebClient.h>
#include <ESP-Common.h>
#include <ESP-Config.h>


char iteration;

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
  checkUpdates();
}

void loop() {
  // first run aka soft init
  if (firstRun == false) {
    firstRun = true;
    Serial.println(F("[*] Exec."));
    Timer0.attach_ms(cfgTimer0ms, timer0Callback);
    Timer1.attach_ms(cfgTimer1ms, timer1Callback);
    Timer2.attach(cfgTimer2s, timer2Callback);
  }

  // Fast timer
  if (tickTimer0 == true) {
#if Enable_WiFiClient == 1
    if (WiFiClientConnected) {
      digitalWrite(LED1, HIGH);
    }
#endif
    digitalWrite(LED2, HIGH);
    Timer0.detach();
    tickTimer0 = false;
  }

  // Slow timer (1s)
  if (tickTimer1 == true) {
    if (DEBUG) {
      Serial.print("tick Timer1 loopCounter=");
      Serial.println(loopCounter);
    }
#if Enable_WiFiClient == 1
    if (WiFiClientConnected) {
      digitalWrite(LED1, LOW);
    }
#endif
    digitalWrite(LED2, LOW);
    Timer0.attach_ms(cfgTimer0ms, timer0Callback);

#if Enable_WebClient == 1
    iteration++;
    if (iteration == ControlServerStep) {
      ControlServerRegister();
      iteration = 0;
    }
#endif
    loopCounter = 0;
    tickTimer1 = false;
  }

  // Very slow timer (60s)
  if (tickTimer2 == true) {
    if (DEBUG) {
      Serial.print("tick Timer2 loopCounter=");
      Serial.println(loopCounter);
    }
#if Enable_WebClient == 1
    Serial.println("[-] Unregistering Control Servers");
    for (auto& j : ControlServersStatus) {
      if (j == 1) {
        j = 0;
      }
    }

    if ((sysMinutes % cfgControlServersRetryMinutes) == 0) {
      connectWiFi();
      Serial.println("[-] Enabling Disabled Control Servers");
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
