# ESP-Module #


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
