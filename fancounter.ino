/*********
  Knusperpony Fan Counter
  Author: LB
*********/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

// BugFix if using ESPAsyncWebserver and WiFiManager --> https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#define WEBSERVER_H
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer

// Set async web server port number to 80
AsyncWebServer server(80);

// Initialize vars
const char *API_TOKEN = "api";

// Initialize Webpage --> Website is saved in Flash not SRAM any more (PROGMEM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
    <title>Knusperpony Fan Counter</title>
    <link rel=\"icon\" href=\"data:,\">
    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
    .button { background-color: #195B6A; border: none; color: white; padding: 16px 40px; text-decoration: none;
    font-size: 30px; margin: 2px; cursor: pointer;}
    </style>
</head>

<body>
    <h1>Knusperpony Fan Counter</h1>
    <label for=\"name\">Facebok API Token:</label>
    <input type=\"text\" id=\"name\" name=\"name\">
    <p><a href=\"/WifiReset\"><button class=\"button\">Delete Wifi Credentials</button></a></p>
</body>

</html>
)rawliteral";

// Function for Deleting WifiCredentials
void EraseWifiCredentials()
{
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(300);
    ESP.restart();
    delay(300);
}

// If there is no Website give back 404
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

// Setup with Wifi Manager, open WifiManager when no AP is yet entered. Open own AP with "Knusperpony"
void setup()
{
    Serial.begin(115200);

    // WiFiManager
    // Local intialization of WifiManager
    WiFiManager wifiManager;
    // takes too long to find WLAN, set timer higher
    wifiManager.setTimeout(30);
    // Open Knusperpony WLAN
    wifiManager.autoConnect("KnusperPony");
    Serial.println("Connected.");

    // Webpage
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html); });

    server.begin();
}
void loop()
{
}
