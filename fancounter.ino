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

#include <Hash.h>
#include <FS.h>

// Set async web server port number to 80
AsyncWebServer server(80);

// Initialize vars
const char *API_KEY = "api_key";

// Initialize Webpage --> Website is saved in Flash not SRAM any more (PROGMEM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <meta name="viewport\" content="width=device-width, initial-scale=1">
    <title>Knusperpony Fan Counter</title>
    <link rel="icon" href="data:,">
    <style>
        html {
            font-family: Helvetica;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
        }

        .button {
            background-color: #195B6A;
            border: none;
            color: white;
            padding: 16px 40px;
            text-decoration: none;
            font-size: 30px;
            margin: 2px;
            cursor: pointer;
        }
    </style>
    <script>
        function submitMessage() {
            alert("Saved value to file system.");
            setTimeout(function () { document.location.reload(false); }, 500);
        }
    </script>

</head>

<body>
    <h1>Knusperpony Fan Counter</h1>
    <label for="name">Facebok API Token:</label>
    <form action="/set_api">
        <input type="text" id="name" name="api_key" value="%api_key%">
        <input type="submit" value="Submit" onclick="submitMessage()">
    </form>
    <br>

    <form action="/WifiReset">
        <button class="button">Delete Wifi Credentials</button>
    </form>
</body>

</html>
)rawliteral";

// Function for Deleting WifiCredentials
void EraseWifiCredentials()
{
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    Serial.println("Wifi Credentials Deleted");
    delay(300);
    ESP.restart();
}

// Read Files from the file system --> https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/examples/SD_Test/SD_Test.ino
String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- empty file or failed to open file");
        return String();
    }
    Serial.println("- read from file:");
    String fileContent;
    while (file.available())
    {
        fileContent += String((char)file.read());
    }
    file.close();
    Serial.println(fileContent);
    return fileContent;
}

// Write file to the file system --> https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/examples/SD_Test/SD_Test.ino
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

// If there is no Website give back 404
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}
// Replaces what stands between % % with the content of api_key.txt
String processor(const String &var)
{
    // Serial.println(var);
    if (var == "api_key")
    {
        return readFile(SPIFFS, "/api_key.txt");
    }
    return String();
}

// Setup with Wifi Manager, open WifiManager when no AP is yet entered. Open own AP with "Knusperpony"
void setup()
{
    Serial.begin(115200);
    // Initialize SPIFFS/Filesystem

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // WiFiManager
    // Local intialization of WifiManager
    WiFiManager wifiManager;
    // takes too long to find WLAN, set timer higher
    wifiManager.setConnectTimeout(180);
    wifiManager.setTimeout(60);
    // Open Knusperpony WLAN
    wifiManager.autoConnect("KnusperPony");
    Serial.println("Connected.");

    // Start Landing Webpage
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    server.on("/WifiReset", HTTP_GET, [](AsyncWebServerRequest *request)
              { EraseWifiCredentials(); });

    // Send a HTTP GET request to xxx.xxx.xxx/get?api_key=<inputMessage>
    server.on("/set_api", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String inputMessage;
        String inputParam;
        // GET api_key value on xxx.xxx.xxx/get?api_key=<inputMessage>
        if (request->hasParam(API_KEY))
        {
            inputMessage = request->getParam(API_KEY)->value();
            writeFile(SPIFFS, "/api_key.txt", inputMessage.c_str());
        }
        else
        {
            inputMessage = "No message sent";
        }
        Serial.println(inputMessage);
        request->send(200, "text/text", inputMessage); });
    server.onNotFound(notFound);
    server.begin();
}

void loop()
{
}
