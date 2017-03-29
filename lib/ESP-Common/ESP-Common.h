#ifndef ESP_COMMON_H
#define ESP_COMMON_H

#include <Ticker.h>
#include <ArduinoJson.h>

extern bool tickTimer0;
extern bool tickTimer1;
extern bool tickTimer2;
extern Ticker Timer0;
extern Ticker Timer1;
extern Ticker Timer2;
extern char sysSeconds;
extern char sysMinutes;
extern uint loopCounter;

extern String cfgAPMAC;
extern String cfgClientMAC;
extern String mySSID;
extern String myPassword;
extern bool WiFiClientConnected;



void timer0Callback();
void timer1Callback();
void timer2Callback();
String formatBytes(size_t bytes);
const char* ModeToString(char m);
const char* ResetReasonToString(char r);
void checkUpdates();
bool assign(const JsonObject& json, const char* keyStr, int &value);
bool assign(const JsonObject& json, const char* keyStr, bool &value);
bool assign(const JsonObject& json, const char* keyStr, String &value, int idx);
bool assign(const JsonObject& json, const char* keyStr, int &value, int idx);
bool JSONLoad();
bool JSONSave();
void GPIOInit(bool prn);
#endif
