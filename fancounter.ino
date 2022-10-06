/*********
  Knusperpony Fan Counter
  Author: LB
*********/
#include <string>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

// BugFix if using ESPAsyncWebserver and WiFiManager --> https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#define WEBSERVER_H
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer

// Use Little Fs instead SPIFFS
#include <Hash.h>
#include <LittleFS.h>
#define SPIFFS LittleFS

// Matrix
//#define FASTLED_ESP8266_D1_PIN_ORDER
//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
//#include "font.h"
//#include <FreeMono9pt7b.h>

const int MATRIX_PIN = D2;

// Set async web server port number to 80
AsyncWebServer server(80);

// Initialize vars
const char *API_KEY = "api_key";
const char *USER_ID = "user_id";
const char *BRIGHTNESS = "brightness";
const char *SECOND_ACC = "second_account";
const char *START_FOLLOWER_COUNT = "start_follower_count";
const char *ACTIVATE_SECOND_ACC = "activate_second_account";

unsigned long lastTime = 0;

// Set delay for api request to 20 seconds (200 allowed requests per hour)
unsigned long timerDelay = 20000;
String graph_facebook = "https://graph.facebook.com/v14.0/";
String api_follower = "?fields=followers_count&access_token=";
String api_follower_second_acount1 = "?fields=followers_count%2Cbusiness_discovery.username(";
String api_follower_second_acount2 = ")%7Bfollowers_count%7D&access_token=";
String follower_request;
int follower_count;
int last_follower_count;
int count_trees = 0;
const String text;

struct Config
{
    String apikey;
    String user_id;
    int brightness;
    String activate_second_account;
    String second_account;
    int start_follower_count;
};

Config config;

#define mw 32
#define mh 8
#define NUMMATRIX (mw * mh)

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, mw, mh,
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
            alert("Saved Settings to Config.");
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
        function secondUserStatus() {
            // Get the checkbox
            var checkBox = document.getElementById("checkbox_activate_secomnd_user");
            // Get the output text
            var block = document.getElementById("second_account");
            var hidden = document.getElementById("hidden_bool")

            // If the checkbox is checked, display the output text
            if (checkBox.checked == true) {
                block.style.display = "initial";
                hidden.value = "true"
            } else {
                block.style.display = "none";
                hidden.value = "false"
                block.value = ""
            }
        }
    </script>

</head>

<body>
    <h1>Knusperpony Fan Counter</h1>
    <label>Facebok API Token:</label>
    <form action="/set_config">
        <input type="text" name="user_id" value="%user_id%">
        <input type="password" name="api_key" value="%api_key%">
        <input type="checkbox" onclick="showPW()">Show API Key
        <br>
        <label>Second User: <input type="checkbox" id="checkbox_activate_secomnd_user"
                onclick="secondUserStatus()"></label>
        <input type="text" style="display:none" id="second_account" name="second_account" value="%second_account%">
        <input type="hidden" id="hidden_bool" name="activate_second_account" value="%activate_second_account%">
        <br>
        <label>Start Follower count for Tree Calculation:</label>
        <input type="text" name="start_follower_count" value="%start_follower_count%">
        <br>
        <label>Brightness:</label>
        <input type="text" name="brightness" value="%brightness%">
        <br>
        <input type="submit" value="Save Settings" onclick="submitMessage()" class="button"/>
    </form><br>

    <br><br><br>
    <br>


    <form action="/WifiReset">
        <button class="button">Delete Wifi Credentials</button>
    </form>


    <script type="text/javascript">
        window.onload = onPageLoad();

        function onPageLoad() {
            var block = document.getElementById("second_account");
            var checker = document.getElementById("checkbox_activate_secomnd_user");

            if ("%activate_second_account%" == "true") {
                checker.checked = true;
                block.style.display = "initial";

            } else {
                checker.checked = false;
                block.value = "";
            }
        }
    </script>
</body>

</html>
)rawliteral";

String btos(bool x)
{
    if (x)
        return "true";
    return "false";
}

// Bitmaps [1]: Instagram Logo, [2] Baum
static const uint16_t PROGMEM bmpArray[][64] =
    {
        {0, 14783, 25023, 37375, 37375, 49598, 49598, 0, 35294, 35294, 65535, 65535, 65535, 65535, 63999, 63928, 53791, 65535, 63928, 63928, 63928, 65535, 65535, 63928, 61983, 65535, 63928, 65535, 65535, 63928, 65535, 63928, 59885, 65535, 59885, 65535, 65535, 63928, 65535, 63928, 64518, 65535, 64518, 64518, 64518, 59885, 65535, 63928, 64901, 65260, 65535, 65535, 65535, 65535, 59885, 63928, 0, 65461, 65461, 65166, 64967, 62502, 64166, 0},
        {0, 11941, 35491, 11941, 11941, 21834, 35491, 0, 11941, 21834, 11941, 35491, 21834, 35491, 11941, 11941, 0, 35491, 21834, 35491, 35491, 11941, 21834, 11941, 0, 11941, 35491, 35491, 11941, 21834, 11941, 0, 0, 0, 11941, 35491, 11941, 0, 0, 0, 0, 0, 0, 35491, 0, 0, 0, 0, 0, 0, 0, 35491, 0, 0, 0, 0, 0, 0, 35491, 35491, 35491, 35491, 0, 0},
};

// Function for Deleting WifiCredentials
void EraseWifiCredentials()
{
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    Serial.println("Wifi Credentials Deleted");
    delay(300);
    ESP.restart();
}

void loadConfiguration(fs::FS &fs, const char *filename)
{
    // Open file for reading
    File file = fs.open(filename, "r");

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
        Serial.println(F("Failed to read file, using default configuration"));

    config.apikey = doc["apikey"].as<String>();
    config.user_id = doc["user_id"].as<String>();

    config.brightness = doc["brightness"];
    config.start_follower_count = doc["start_follower_count"];
    config.activate_second_account = doc["activate_second_account"].as<String>();
    config.second_account = doc["second_account"].as<String>();
    // Close the file (Curiously, File's destructor doesn't close the file)
    Serial.println(config.apikey);
    Serial.println(config.user_id);
    file.close();
}

void saveConfiguration(fs::FS &fs, const char *filename, const Config &config)
{
    // Open file for writing
    File file = fs.open(filename, "w");
    if (!file)
    {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<384> doc;

    doc["apikey"] = config.apikey;
    doc["user_id"] = config.user_id;
    doc["brightness"] = config.brightness;
    doc["start_follower_count"] = config.start_follower_count;
    doc["activate_second_account"] = config.activate_second_account;
    doc["second_account"] = config.second_account;

    // Set the values in the document
    // doc["hostname"] = config.hostname;
    // doc["port"] = config.port;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
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
// Replaces what stands between % % with the content of config.json
String processor(const String &var)
{
    // Serial.println(var);
    if (var == "api_key")
    {
        return config.apikey;
    }
    else if (var == "user_id")
    {
        return config.user_id;
    }
    else if (var == "brightness")
    {
        return String(config.brightness);
    }
    else if (var == "second_account")
    {
        return config.second_account;
    }
    else if (var == "start_follower_count")
    {
        return String(config.start_follower_count);
    }
    else if (var == "activate_second_account")
    {
        return config.activate_second_account;
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

int getFollowerCount(const char *request)
{
    int followers_count;
    StaticJsonDocument<192> doc;

    WiFiClientSecure client;
    HTTPClient http;
    // better solution, get root certificates from mozilla and use tsl handshake (set CPU to 160 MHZ)
    client.setInsecure();
    http.useHTTP10(true);

    //  client.connect(host, httpsPort);
    //   Your IP address with path or Domain name with URL path
    http.begin(client, request);
    // Serial.println(serverName);
    //  Send HTTP POST request
    int httpResponseCode = http.GET();

    String stringload = "{}";

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        deserializeJson(doc, http.getStream());

        followers_count = doc["followers_count"].as<int>() + doc["business_discovery"]["followers_count"].as<int>();

        Serial.println(followers_count);
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return followers_count;
}

void displayBMPText(int bmp, String text, int duration)
{
    matrix->clear();
    matrix->drawRGBBitmap(0, 0, bmpArray[bmp], 8, 8);
    matrix->setCursor(9, 1);
    matrix->print(text);
    matrix->show();
    FastLED.delay(duration);
}

void scrollBMP(uint8_t bitmapSize, int bmp) //-->https://github.com/marcmerlin/FastLED_NeoMatrix/blob/master/examples/MatrixGFXDemo/MatrixGFXDemo.ino#L517
{
    // set starting point at zero
    int16_t xf = 0;
    int16_t yf = 0;
    // scroll speed in 1/16th
    int16_t xfc = 4;
    int16_t yfc = 3;
    // scroll down and right by moving upper left corner off screen
    // more up and left (which means negative numbers)
    int16_t xfdir = -1;
    int16_t yfdir = -1;

    for (uint16_t i = 1; i < 500; i++)
    {
        bool updDir = false;

        // Get actual x/y by dividing by 16.
        int16_t x = xf >> 4;
        int16_t y = yf >> 4;

        matrix->clear();
        // bounce 8x8 tri color smiley face around the screen
        if (bitmapSize == 8)
            matrix->drawRGBBitmap(x, y, bmpArray[bmp], 8, 8);

        matrix->show();

        // Only pan if the display size is smaller than the pixmap
        // but not if the difference is too small or it'll look bad.
        if (bitmapSize - mw > 2)
        {
            xf += xfc * xfdir;
            if (xf >= 0)
            {
                xfdir = -1;
                updDir = true;
            };
            // we don't go negative past right corner, go back positive
            if (xf <= ((mw - bitmapSize) << 4))
            {
                xfdir = 1;
                updDir = true;
            };
        }
        if (bitmapSize - mh > 2)
        {
            yf += yfc * yfdir;
            // we shouldn't display past left corner, reverse direction.
            if (yf >= 0)
            {
                yfdir = -1;
                updDir = true;
            };
            if (yf <= ((mh - bitmapSize) << 4))
            {
                yfdir = 1;
                updDir = true;
            };
        }
        // only bounce a pixmap if it's smaller than the display size
        if (mw > bitmapSize)
        {
            xf += xfc * xfdir;
            // Deal with bouncing off the 'walls'
            if (xf >= (mw - bitmapSize) << 4)
            {
                xfdir = -1;
                updDir = true;
            };
            if (xf <= 0)
            {
                xfdir = 1;
                updDir = true;
            };
        }
        if (mh > bitmapSize)
        {
            yf += yfc * yfdir;
            if (yf >= (mh - bitmapSize) << 4)
            {
                yfdir = -1;
                updDir = true;
            };
            if (yf <= 0)
            {
                yfdir = 1;
                updDir = true;
            };
        }

        if (updDir)
        {
            // Add -1, 0 or 1 but bind result to 1 to 1.
            // Let's take 3 is a minimum speed, otherwise it's too slow.
            xfc = constrain(xfc + random(-1, 2), 3, 16);
            yfc = constrain(xfc + random(-1, 2), 3, 16);
        }
        FastLED.delay(10);
    }
}
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
    wifiManager.setTimeout(180);
    // Open Knusperpony WLAN
    wifiManager.autoConnect("KnusperPony");
    Serial.println("Connected.");

    loadConfiguration(SPIFFS, "/config.json");

    if (config.activate_second_account == "true")
    {
        follower_request = graph_facebook + config.user_id + api_follower_second_acount1 + config.second_account + api_follower_second_acount2 + config.apikey;
    }
    else
    {
        follower_request = graph_facebook + config.user_id + api_follower + config.apikey;
    }
    follower_count = getFollowerCount(follower_request.c_str());
    count_trees = (follower_count - config.start_follower_count) / 50;
    Serial.println(follower_request);

    // Start Landing Webpage, calls the processor function on the index_html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    server.on("/WifiReset", HTTP_GET, [](AsyncWebServerRequest *request)
              { EraseWifiCredentials(); });

    // Send a HTTP GET request to xxx.xxx.xxx/get?api_key=<inputMessage>
    server.on("/set_config", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  // GET api_key value on xxx.xxx.xxx/get?api_key=<inputMessage>
                  if (request->hasParam(API_KEY) && request->hasParam(USER_ID))
                  {
                      Serial.println("I got API & USER ID");
                      config.apikey = request->getParam(API_KEY)->value();
                      config.user_id = request->getParam(USER_ID)->value();
                  }

                  if (request->hasParam(BRIGHTNESS))
                  {
                      Serial.println("I got a brightness value");
                      config.brightness = (request->getParam(BRIGHTNESS)->value()).toInt();
                      Serial.println(config.brightness);
                  }

                  if (request->hasParam(START_FOLLOWER_COUNT))
                  {
                      config.start_follower_count = (request->getParam(START_FOLLOWER_COUNT)->value()).toInt();
                      Serial.println(config.start_follower_count);
                  }

                  if (request->hasParam(ACTIVATE_SECOND_ACC))
                  {
                      config.activate_second_account = (request->getParam(ACTIVATE_SECOND_ACC)->value());
                      Serial.println(config.activate_second_account);
                  }

                  if (request->hasParam(SECOND_ACC))
                  {
                      config.second_account = (request->getParam(SECOND_ACC)->value());
                      Serial.println(config.second_account);
                  }
                  saveConfiguration(SPIFFS, "/config.json", config);
                  if (config.activate_second_account == "true")
                  {
                      follower_request = graph_facebook + config.user_id + api_follower_second_acount1 + config.second_account + api_follower_second_acount2 + config.apikey;
                  }
                  else
                  {
                      follower_request = graph_facebook + config.user_id + api_follower + config.apikey;
                  }
                  count_trees = (follower_count - config.start_follower_count) / 50;
                  matrix->setBrightness(config.brightness);
                  Serial.println(follower_request); });

    server.onNotFound(notFound);
    server.begin();

    FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(matrixleds, NUMMATRIX);
    matrix->begin();
    matrix->setTextWrap(false);
    // matrix->setFont(&PixelItFont);
    matrix->setBrightness(config.brightness);
    matrix->setTextColor(matrix->Color(255, 255, 255));
    matrix->print("Knuspercount");
    matrix->show();
    FastLED.delay(5000);
    matrix->clear();
}

void loop()
{

    // Send an HTTP POST request depending on timerDelay
    if ((millis() - lastTime) > timerDelay || lastTime == 0)
    {
        follower_count = getFollowerCount(follower_request.c_str());

        if ((follower_count - config.start_follower_count) % 50 == 0 && follower_count != last_follower_count)
        {
            scrollBMP(8, 1);
            count_trees = (follower_count - config.start_follower_count) / 50;
            last_follower_count = follower_count;
        }
        lastTime = millis();
    }

    displayBMPText(0, String(follower_count), 20000);

    displayBMPText(1, "x" + String(count_trees), 5000);
}
