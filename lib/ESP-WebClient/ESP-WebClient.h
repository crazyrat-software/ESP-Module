#ifndef ESP_WEBCLIENT_H
#define ESP_WEBCLIENT_H

extern HTTPClient WebClient;
extern char httpcode;

extern int requestURL(String url);
extern void ControlServerRegister();

#endif
