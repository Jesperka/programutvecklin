#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"
#include <SPI.h>
#include "pin_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

// WiFi credentials
String ssid = "#Telia-EBB130";
String password = "1KTSASA6K71fT1d7";

TFT_eSPI tft = TFT_eSPI();
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170

WiFiClient wifi_client;
//Visual function for the menu
void drawMenu(int selectedIndex) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Menu", DISPLAY_WIDTH / 2, 10, 2);
  //An array of strings (options) is displayed, and shows selected through a >
  String menuItems[2] = {"Forecast", "Settings"};
  for (int i = 0; i < 2; i++) {
    int x = 30;
    int y = 50 + i * 30;
    if (i == selectedIndex) {
      tft.drawString(">", x - 20, y);
    }
    tft.drawString(menuItems[i], x, y);
  }
}

String showMenu() {
  int selected = 0;
  String menuItems[2] = {"Forecast", "Settings"};
  unsigned long lastInputTime = millis();
  const unsigned long timeout = 5000;

  drawMenu(selected);

  while (true) {
    int knapp1 = digitalRead(PIN_BUTTON_1);
    int knapp2 = digitalRead(PIN_BUTTON_2);

    if (knapp1 == LOW) {
      selected--;
      if (selected < 0) selected = 1;
      drawMenu(selected);
      lastInputTime = millis();
      delay(200);
    }

    if (knapp2 == LOW) {
      selected++;
      if (selected > 1) selected = 0;
      drawMenu(selected);
      lastInputTime = millis();
      delay(200);
    }

    if (millis() - lastInputTime >= timeout) {
      return menuItems[selected];
    }
    delay(10);
  }
}

String SmhiData(int city) {
  String cityNames[3] = {"Karlskrona", "Goteborg", "Stockholm"};
  String urls[3] = {
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.59/lat/56.18/data.json", // Karlskrona
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/11.97/lat/57.7/data.json",  // Göteborg
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/18.07/lat/59.32/data.json"  // Stockholm
  };

  if (city < 0 || city > 2) return "Invalid city index";

  String cityName = cityNames[city];
  String url = urls[city];

  Serial.println("Fetching SMHI for " + cityName);
  Serial.println("URL: " + url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    WiFiClient& stream = http.getStream();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, stream);


    if (error) {
      Serial.println("JSON error: " + String(error.c_str()));
      http.end();
      return "JSON error";
    }

    JsonArray timeSeries = doc["timeSeries"];
    if (timeSeries.size() == 0) {
      Serial.println("No timeSeries data!");
      http.end();
      return "No timeSeries";
    }

    for (int i = 0; i < min(3, (int)timeSeries.size()); i++) {
      JsonArray parameters = timeSeries[i]["parameters"];
      for (JsonObject param : parameters) {
        if (param["name"] == "t") {
          float temperature = param["values"][0];
          String time = timeSeries[i]["validTime"];
          http.end();
          Serial.println("Temp found: " + String(temperature));
          return cityName + ": " + String(temperature, 1) + "°C";
        }
      }
    }

    http.end();
    return "Temp not found";
  } else {
    Serial.println("HTTP error: " + String(httpCode));
    http.end();
    return "HTTP error";
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Connecting to WiFi...", 10, 10);
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Connected to WiFi", 10, 10);
  delay(1000);
}

void loop() {
  static int initDone = 0;
  static int city = 1;
  String x;

  if (!initDone) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Version 1.0", 50, 40);
    tft.drawString("Group 4", 60, 60);
    delay(3000);
    initDone = 1;
  }

  x = showMenu();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (x == "Forecast") {
    tft.drawString(SmhiData(city), 30, 100);
    delay(10000);
  } else if (x == "Settings") {
    tft.drawString("Settings", 110, 100);
    delay(5000);
  }

  // Optional: return to menu after displaying result
}

// TFT Pin check
//////////////////
// DO NOT TOUCH //
//////////////////
#if PIN_LCD_WR  != TFT_WR || \
    PIN_LCD_RD  != TFT_RD || \
    PIN_LCD_CS    != TFT_CS   || \
    PIN_LCD_DC    != TFT_DC   || \
    PIN_LCD_RES   != TFT_RST  || \
    PIN_LCD_D0   != TFT_D0  || \
    PIN_LCD_D1   != TFT_D1  || \
    PIN_LCD_D2   != TFT_D2  || \
    PIN_LCD_D3   != TFT_D3  || \
    PIN_LCD_D4   != TFT_D4  || \
    PIN_LCD_D5   != TFT_D5  || \
    PIN_LCD_D6   != TFT_D6  || \
    PIN_LCD_D7   != TFT_D7  || \
    PIN_LCD_BL   != TFT_BL  || \
    TFT_BACKLIGHT_ON   != HIGH  || \
    170   != TFT_WIDTH  || \
    320   != TFT_HEIGHT
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif
