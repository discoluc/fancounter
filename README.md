# Fancounter
The goal is to build a social media fan counter using an ESP8266 and a 32 x 8 Full RGB Pixelmatrix.

## Prerequisities/Libraries
- [ESP8266](https://arduino.esp8266.com/stable/package_esp8266com_index.json)
- [WifiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJSONLibrary](https://github.com/bblanchon/ArduinoJson)


## ToDos

- <s>Using WifiManager to set up Wifi</s>
- <s>Opening webserver for basic setting display</s>
- Make Webpage for entering the api key and resetting the wifi creds
- make save api key in SPIFFS
- Integrate Facebook API Request
    https://graph.facebook.com/v14.0/{ownuserprofileid}?fields=followers_count&access_token={apikey}
    https://graph.facebook.com/v14.0/{ownuserprofileid}?fields=business_discovery.username({seconduser})%7Bfollowers_count%7D&access_token={apikey}

- Use FastLED Library for visualization of Fan Count & Instagram Logo as Pixelart

