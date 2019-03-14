#ifndef ESP_WEBSERVER_H
#define ESP_WEBSERVER_H

#include <Arduino.h>
#include <FS.h>
#include "ESP-Common.h"
#include "ESP-Config.h"
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;
extern File fsUploadFile;


int getPinFromURL(String url, String prefix);
int getPinValueFromURL(String url, String prefix);
int getPinModeFromURL(String url, String prefix);
String getContentType(String filename);
String header();
String footer();
void startWebserver();
void handleRoot();
bool handleFileRead(String path);
void handleFileList();
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
void handleShowStatus();
void handleShowState();
void handleConfigLoad();
void handleConfigSave();
void handleSysRestart();
void handleGetGPIO();
void handleSetGPIO();
void handleGetMode();
void handleGetModeStr();
void handleSetMode();

#endif
