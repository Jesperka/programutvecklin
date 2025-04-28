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
String ssid = "Jespers iPhone 14";
String password = "Jesper12";

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
  String urlss[3] = {
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.59/lat/56.18/data.json", // Karlskrona
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/11.97/lat/57.7/data.json",  // Göteborg
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/18.07/lat/59.32/data.json"  // Stockholm
  };
  String urls[3] = {
    "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2711537/combined", // Göteborg
    "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2701713/combined", // Karlskrona
    "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2673730/combined" // Stockholm
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
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);


    if (error) {
      Serial.println("JSON error: " + String(error.c_str()));
      http.end();
      return "JSON error";
    } else {
      float temperature = doc["forecast10d"]["daySerie"][0]["data"][0]["t"].as<float>();
      http.end();
      return String(temperature) + " C i " + cityName;
    }

    
    

    http.end();
    return "Temp not found";
  } else {
    Serial.println("HTTP error: " + String(httpCode));
    http.end();
    return "HTTP error";
  }
}

void drawSettingsMenu(int selectedIndex, int currentCity) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Settings", DISPLAY_WIDTH / 2, 10, 2);

  String settingsItems[2] = {
    "Return",
    "City: " + String((currentCity == 0) ? "Karlskrona" : (currentCity == 1) ? "Goteborg" : "Stockholm")
  };

  for (int i = 0; i < 2; i++) {
    int x = 30;
    int y = 50 + i * 30;
    if (i == selectedIndex) {
      tft.drawString(">", x - 20, y); // Visa ">" framför valt alternativ
    } else {
      tft.drawString(" ", x - 20, y); // Tomt framför icke valda
    }
    tft.drawString(settingsItems[i], x, y);
  }
}

String showSettings(int &city) {
  int selected = 0;
  unsigned long selectedTime = 0;
  bool holding = false;

  drawSettingsMenu(selected, city);

  while (true) {
    int knapp1 = digitalRead(PIN_BUTTON_1);
    int knapp2 = digitalRead(PIN_BUTTON_2);

    if (knapp1 == LOW) {
      selected--;
      if (selected < 0) selected = 1;
      drawSettingsMenu(selected, city);
      holding = false;
      delay(200);
    }

    if (knapp2 == LOW) {
      selected++;
      if (selected > 1) selected = 0;
      drawSettingsMenu(selected, city);
      holding = false;
      delay(200);
    }

    if (knapp1 == HIGH && knapp2 == HIGH) {
      if (!holding) {
        holding = true;
        selectedTime = millis();
      } else if (millis() - selectedTime >= 3000) { // 3 sekunders hållning
        if (selected == 0) { // Return
          return "Return";
        } else if (selected == 1) { // City
          city = (city + 1) % 3; // Växla stad
          drawSettingsMenu(selected, city); // Rita om ny stad
          holding = false;
          delay(300);
        }
      }
    } else {
      holding = false;
    }

    delay(10);
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
    String s = showSettings(city);
    if (s == "Return") {
      // Gå tillbaka till meny
    }
  }
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
