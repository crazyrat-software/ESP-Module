# ESP-Module #


## Features ##
1. Wireless client connects up to 5 configured networks
2. Wireless client reconnects if connection is lost
3. Access point enabled with unique SSID
4. Webserver
  1. Status and simple controls
  2. Rest methods returns JSON structures
  3. Simple edit / fileserver for local SPIFFS. Can be used to change stored configuration.
5. WebClient
  1. Registers up to 2 Control Servers
  2. Constantly monitor health of Control Servers
6. Predefined update server is checked during startup
