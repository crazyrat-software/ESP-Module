#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#include <Arduino.h>
#include <Esp.h>
#include <ESP8266WiFi.h>


/*
     Global configuration
*/

#define VERSION "1.4"
#define Update_Server "http://crazyrat.pl/esp/update.php"
extern bool DEBUG;
extern bool firstRun;
extern const char *cfgMachine;
extern const char *cfgSSID;
extern const char *cfgPASSWORD;
extern const IPAddress cfgAPIP;
extern const IPAddress cfgAPGateway;
extern const IPAddress cfgAPMask;
constexpr int cfgListenPort = 80;

// pins defined to be used on board
constexpr int PinsCount = 11;
extern int cfgPinsMode[PinsCount];
const int cfgPins[PinsCount] = {D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10};
extern String cfgPinsLabels[PinsCount];
// one of: INPUT=0, INPUT_PULLUP, OUTPUT=1, -1 = don't initialize,
extern int cfgPinsMode[PinsCount];

constexpr char LED1 = D4;
constexpr char LED2 = D0;
constexpr char ButtonFLASH = D3;
constexpr uint cfgTimer0ms = 1000;       // tick every second
constexpr uint cfgTimer1ms = 10;
constexpr char cfgTimer2s = 60;          // tick every minute
extern String html;
extern String tmpStr;
constexpr int ControlServersCount = 2;
extern String cfgControlServers[ControlServersCount];
extern int cfgControlServersRetryCount;      //disabling after this number of retries
extern int cfgControlServersRetryMinutes;   //retry disabled servers (minutes)
// -100=disabled; <0=error registration; 0=unregistered; 1=registered
extern int ControlServersStatus[ControlServersCount];
extern char ControlServersIndex;             //currently requested ControlServer
extern char ControlServerStep;               //request once per step (1s)

constexpr char WiFiSSIDsCount = 5;
extern String cfgWiFiSSIDs[WiFiSSIDsCount];
extern String cfgWiFiPasswords[5];
constexpr int maxConfigFileSize = 1024;
#endif
