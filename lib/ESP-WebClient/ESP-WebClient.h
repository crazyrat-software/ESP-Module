#ifndef ESP_WEBCLIENT_H
#define ESP_WEBCLIENT_H

extern HTTPClient WebClient;
extern char httpcode;

extern int requestGET(String url);
extern int requestPOST(String url, String data);
extern void ControlServerRegister();
extern void ControlServerPushData();
#endif
