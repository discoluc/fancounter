/*********
  Knusperpony Fan Counter
  Author: LB
*********/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

// BugFix if using ESPAsyncWebserver and WiFiManager --> https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#define WEBSERVER_H
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer

// Use Little Fs instead SPIFFS
#include <Hash.h>
#include <LittleFS.h>
#define SPIFFS LittleFS

// Fastled
//#define FASTLED_ESP8266_D1_PIN_ORDER
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include "font.h"

const int MATRIX_PIN = D2;

// Set async web server port number to 80
AsyncWebServer server(80);

// Initialize vars
const char *API_KEY = "api_key";
const char *USER_ID = "user_id";

unsigned long lastTime = 0;
// Set timer to 40 seconds (40000)
unsigned long timerDelay = 40000;
String graph_facebook = "https://graph.facebook.com/v14.0/";
String api_follower = "?fields=followers_count&access_token=";
String second_account = "ridersfuture";
String api_business_discovery = "?fields=business_discovery.username(" + second_account + ")%7Bfollowers_count%7D&access_token=";
String follower_JSON;
String follower_request;
int follower_count_inc;
int follower_count;
const String text;

#define mw 8
#define mh 32
#define NUMMATRIX (mw * mh)

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, mh, mw,
                                                  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);

// Initialize Webpage --> Website is saved in Flash not SRAM any more (PROGMEM)
const char index_html[] PROGMEM =
    R"rawliteral(
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
            padding: 12px 24px;
            text-decoration: none;
            font-size: 12px;
            margin: 2px;
            cursor: pointer;
        }
    </style>
    <script>
        function submitMessage() {
            alert("Saved User ID + API Key to file system.");
            setTimeout(function () { document.location.reload(false); }, 500);
        }

        function showPW() {
            var x = document.getElementById("api_key_field");
            if (x.type === "password") {
                x.type = "text";
            } else {
                x.type = "password";
            }
        }
    </script>

</head>

<body>
    <h1>Knusperpony Fan Counter</h1>
    <label for="name">Facebok API Token:</label>
    <form action="/set_api">
        <input type="text" id="user_id_field" name="user_id" value="%user_id%">
        <input type="password" id="api_key_field" name="api_key" value="%api_key%">
        <input type="submit" value="Submit" onclick="submitMessage()">
    </form>
    <br>
    <input type="checkbox" onclick="showPW()">Show API Key
    <br>
    <br>
    <br>
    <form action="/WifiReset">
        <button class="button">Delete Wifi Credentials</button>
    </form>
</body>

</html>
)rawliteral";

static const uint16_t bmpArray[64] PROGMEM =
    {
        0, 14783, 25023, 37375, 37375, 49598, 49598, 0, 35294, 35294, 65535, 65535, 65535, 65535, 63999, 63928, 53791, 65535, 63928, 63928, 63928, 65535, 65535, 63928, 61983, 65535, 63928, 65535, 65535, 63928, 65535, 63928, 59885, 65535, 59885, 65535, 65535, 63928, 65535, 63928, 64518, 65535, 64518, 64518, 64518, 59885, 65535, 63928, 64901, 65260, 65535, 65535, 65535, 65535, 59885, 63928, 0, 65461, 65461, 65166, 64967, 62502, 64166, 0};

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
    else if (var == "user_id")
    {
        return readFile(SPIFFS, "/user_id.txt");
    }
    return String();
}

String httpGETRequest(const char *serverName)
{
    WiFiClientSecure client;
    HTTPClient http;
    // better solution, get root certificates from mozilla and use tsl handshake (set CPU to 160 MHZ)
    client.setInsecure();
    //  client.connect(host, httpsPort);
    //   Your IP address with path or Domain name with URL path
    http.begin(client, serverName);
    // Serial.println(serverName);
    //  Send HTTP POST request
    int httpResponseCode = http.GET();

    String stringload = "{}";

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        stringload = http.getString();
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return stringload;
}

// Setup with Wifi Manager, open WifiManager when no AP is yet entered. Open own AP with "Knusperpony"
void setup()
{
    Serial.begin(115200);
    // Initialize SPIFFS/Filesystem (remember SPIFFS is now LittleFS)

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
    String api_key = readFile(SPIFFS, "/api_key.txt");
    String user_id = readFile(SPIFFS, "/user_id.txt");
    follower_request = graph_facebook + user_id + api_follower + api_key;
    Serial.println(follower_request);

    // Start Landing Webpage, calls the processor function on the index_html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    server.on("/WifiReset", HTTP_GET, [](AsyncWebServerRequest *request)
              { EraseWifiCredentials(); });

    // Send a HTTP GET request to xxx.xxx.xxx/get?api_key=<inputMessage>
    server.on("/set_api", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String apimsg;
        String usermsg;
        // GET api_key value on xxx.xxx.xxx/get?api_key=<inputMessage>
        if (request->hasParam(API_KEY)&& request->hasParam(USER_ID))
        {
            apimsg = request->getParam(API_KEY)->value();
            usermsg = request->getParam(USER_ID)->value();
            writeFile(SPIFFS, "/api_key.txt", apimsg.c_str());
            writeFile(SPIFFS, "/user_id.txt", usermsg.c_str()); 
            follower_request = graph_facebook + usermsg + api_follower + apimsg;
            

        }
        else
        {
            apimsg = "No message sent";
        } });
    server.onNotFound(notFound);
    server.begin();
    FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(matrixleds, NUMMATRIX);
    matrix->begin();
    matrix->setTextWrap(false);
    matrix->setFont(&PixelItFont);
    matrix->setBrightness(50);
    matrix->setTextColor(matrix->Color(255, 255, 255));
    matrix->clear();
}

void loop()
{
    // Send an HTTP POST request depending on timerDelay
    if ((millis() + 40001 - lastTime) > timerDelay)
    {
        // Check WiFi connection status
        if (WiFi.status() == WL_CONNECTED)
        {
            follower_JSON = httpGETRequest(follower_request.c_str());
            follower_count = JSON.parse(follower_JSON)["followers_count"];
            follower_count_inc = follower_count + 0;
            Serial.println(follower_count);
        }
        else
        {
            Serial.println("WiFi Disconnected");
        }
        lastTime = millis() + 40001;
        matrix->clear();
    }
    matrix->drawRGBBitmap(0, 0, bmpArray, 8, 8);
    matrix->setCursor(9, 1);
    matrix->print(follower_count_inc);
    matrix->show();
    delay(400);
}
