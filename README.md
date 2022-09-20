# Fancounter

The goal is to build a social media fan counter using an ESP8266 and a 32 x 8 Full RGB Pixelmatrix.

## Prerequisities/Libraries

- [ESP8266](https://arduino.esp8266.com/stable/package_esp8266com_index.json)
- [WifiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJSONLibrary](https://github.com/bblanchon/ArduinoJson)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)


## ToDos

- [x] Using WifiManager to set up Wifi
- [x] Opening webserver for basic setting display
- [x] Make Webpage for entering the api key and resetting the wifi creds
    - [x] using ESPAsynWebserver and the PROGMEM not SRAM anymore
    - [ ] HTTPAccess to Webserver 
- [x] save api key + user id in SPIFFS
- [x] Integrate Facebook API Request
    - [x] HTTP GET Request (insecure)
    - [ ] Make HTTP GET Request secure
    - [x] integrate JSON Parser
- [x] Use FastLED Library for visualization of Fan Count & Instagram Logo as Pixelart

