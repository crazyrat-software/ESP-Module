#include <Arduino.h>
#include <Esp.h>
#include <ESP8266WiFi.h>
#include "ESP-Config.h"

/*
     Global configuration
*/

bool DEBUG = false;
bool firstRun = false;
const char *cfgMachine = "ESP8266";
const char *cfgSSID = "ESP-";
const char *cfgPASSWORD = "P";
const IPAddress cfgAPIP(192, 168, 128, 1);
const IPAddress cfgAPGateway(192, 168, 128, 1);
const IPAddress cfgAPMask(255, 255, 255, 0);

String html = "";
String tmpStr = "";
String cfgControlServers[ControlServersCount] = {"http://192.168.1.238/esp/register.php", "http://crazyrat.pl/esp/register.php"};
int cfgControlServersRetryCount = 5;      //disabling after this number of retries
int cfgControlServersRetryMinutes = 15;   //retry disabled servers (minutes)
// -100=disabled; <0=error registration; 0=unregistered; 1=registered
int ControlServersStatus[ControlServersCount] = {0, 0};
char ControlServersIndex = 0;             //currently requested ControlServer
char ControlServerStep = 2;               //request once per step (1s)
String cfgWiFiSSIDs[5] = {"", "", "", "", ""};
String cfgWiFiPasswords[5] = {"", "", "", "" ,""};

String cfgPinsLabels[] = {"D0 (LED2)", "D1", "D2", "D3 (FLASH)", "D4 (LED1)", "D5", "D6", "D7", "D8", "D9 (RX)", "D10 (TX)"};
int cfgPinsMode[] = {OUTPUT, OUTPUT, OUTPUT, -1, OUTPUT, INPUT, INPUT, INPUT, INPUT, -1, -1};
