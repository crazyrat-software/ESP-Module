# ESP-Module #
This is a abandoned, but working skeleton of ESP controlled SmartDevice infrastructure based on WiFi communication. Multiple features are implemented to help maintain stable communication and OTA (Over The Air) updates from central server. Code provides basic IO functionality to test physical device integration and exposes simple API for building more on top of it.
Have fun, expand, give your thougts and suggestions. Nowadays there are plenty of similar projects, but main their mein idea is still missing: building from scratch gives more satisfaction.

## Features ##
1. Wireless client connects up to 5 configured networks
2. Wireless client reconnects if connection is lost
3. Access point enabled with unique SSID
4. Webserver
  - Status and simple controls
  - Rest methods returns JSON structures  
  - Simple edit / fileserver for local SPIFFS. Can be used to change stored configuration.
5. WebClient
  - Registers up to 2 Control Servers
  - Constantly monitor health of Control Servers
6. Predefined update server is checked during startup
7. Blynk library support added.
