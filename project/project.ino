#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

TFT_eSPI tft = TFT_eSPI();

String ssid = "Jespers iPhone 14";
String password = "Jesper12";

int city = 1;
int tempUnit = 0;
bool showSymbols = true;
//Links and citynames
String cityNames[] = {"Karlskrona", "Goteborg", "Stockholm"};
String urls[] = {
  "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2701713/combined",
  "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2711537/combined",
  "https://wpt-a.smhi.se/backend-weatherpage/forecast/fetcher/2673730/combined"
};
//
struct WeatherData {
  float temperatures[8];
  String symbols[8];
};
//Translate Wsymb2 to symbol
String getSymbolFromCode(int sym) {
  Serial.println(sym);
  if (sym == 1 || sym == 2) return "☀ (Sun) ";      // clear / mostly clear
  if (sym == 3 || sym == 4) return "☁ (Cloud) ";      // partly to fully cloudy
  if (sym >= 5 && sym <= 8) return "☔ (Rain) ";      // different types of rain
  if (sym >= 9 && sym <= 14) return "❄ (Snow) ";     // snow/mixed
  if (sym >= 15 && sym <= 18) return "⚡ (Thunder) ";    // thunderstorm/heavy rain
  return "?";                                // fallback
}
//Getting weather info and translate wsymb2 to symbol
void fetchForecast(WeatherData &data) {
  HTTPClient http;
  http.begin(urls[city]);
  //Check if the connection went correctly
  if (http.GET() == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    //Collect all temperature
    for (int i = 0; i < 8; i++) {
     data.temperatures[i] = doc["forecast10d"]["daySerie"][0]["data"][i]["t"];
    }
    //Collect all symbols
    for (int i = 0; i < 8; i++) {
     int sym = doc["forecast10d"]["daySerie"][0]["data"][i]["Wsymb2"];
     data.symbols[i] = getSymbolFromCode(sym);
    }
  }
  http.end();
}
//Starting screen
void drawBootScreen() {
  WeatherData data;
  fetchForecast(data);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  // Header info
  tft.setTextSize(2);
  tft.drawString("Version 1.0", 10, 10);
  tft.drawString("Group 4", 10, 30);

  // Forecast table
  tft.setTextSize(1);
  for (int i = 0; i < 8; i++) {
    int y = 55 + i * 12; //To get good distances between "temperature and symbol blocks"
    String hour = String(i * 3) + "h";
    String temp = String(data.temperatures[i], 1) + (tempUnit == 0 ? " C" : " F");
    String symbol = showSymbols ? data.symbols[i] : "";

    tft.drawString(hour, 10, y);
    tft.drawString(symbol, 50, y);
    tft.drawString(temp, 160, y);
  }

  delay(4000);
}
//Main menu
void drawMenu(int selected) {
  tft.fillScreen(TFT_BLACK);
  String items[] = {"Forecast", "Settings", "History"};
  tft.setTextSize(2);
  tft.drawCentreString("Menu", 160, 10, 2);
  //The selector and drawer for main menu
  for (int i = 0; i < 3; i++) {
    String prefix = (i == selected) ? "→ " : "  ";
    tft.drawString(prefix + items[i], 10, 50 + i * 30);
  }
  tft.setTextSize(1);
  tft.drawString("B1: Scroll  Hold B2: Select", 10, 140);
}

String showMenu() {
  int selected = 0; //Checks which element from String items that is selected
  unsigned long pressStart = 0;
  drawMenu(selected);
  while (true) {
    if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == HIGH) {
      selected = (selected + 1) % 3; //Changes value to selected
      drawMenu(selected);
      delay(200);
    }
    if (digitalRead(PIN_BUTTON_2) == LOW && digitalRead(PIN_BUTTON_1) == HIGH) {
      if (pressStart == 0) pressStart = millis();
      if (millis() - pressStart > 250) {
        while (digitalRead(PIN_BUTTON_2) == LOW) delay(10);
        return selected == 0 ? "Forecast" : selected == 1 ? "Settings" : "History";
      }
    } else {
      pressStart = 0;
    }
    delay(10);
  }
}

void drawForecast(const WeatherData &data) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Forecast for " + cityNames[city], 10, 10);
  for (int i = 0; i < 8; i++) {
    int y = 30 + i * 15; // To get good distances between the lines
    tft.drawString(String(i * 3) + "h:", 10, y);
    tft.drawString(String(data.temperatures[i], 1) + (tempUnit == 0 ? " C" : " F"), 60, y);
    if (showSymbols) tft.drawString(data.symbols[i], 130, y);
  }
  tft.drawString("Hold both buttons to return", 10, 150);
  delay(10000);
}

void showSettings() {
  int selected = 0;
  unsigned long pressStart = 0, holdStart = 0;
  //A loop to check if the two buttons are pressed
  while (true) {
    if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == LOW) {
      if (holdStart == 0) holdStart = millis();
      if (millis() - holdStart > 2000) return;
    } else {
      holdStart = 0;
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    String opts[] = {
      "Back",
      "City: " + cityNames[city],
      "Unit: " + String(tempUnit == 0 ? "C" : "F"),
      "Symbols: " + String(showSymbols ? "On" : "Off")
    };
    //Drawing
    for (int i = 0; i < 4; i++) {
      String prefix = (i == selected) ? "→ " : "  ";
      tft.drawString(prefix + opts[i], 10, 30 + i * 30);
    }
    tft.setTextSize(1);
    tft.drawString("K1: Scroll  K2: Select", 10, 140);
    // Change value to selected
    if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == HIGH) {
      selected = (selected + 1) % 4;
      delay(200);
    }
    //An if loop to check whats selected and to execute whatever is supposed to be executed
    if (digitalRead(PIN_BUTTON_2) == LOW && digitalRead(PIN_BUTTON_1) == HIGH) {
      if (pressStart == 0) pressStart = millis();
      if (millis() - pressStart > 250) {
        while (digitalRead(PIN_BUTTON_2) == LOW) delay(10);
        if (selected == 0) return;
        else if (selected == 1) city = (city + 1) % 3;
        else if (selected == 2) tempUnit = 1 - tempUnit;
        else if (selected == 3) showSymbols = !showSymbols;
      }
    } else {
      pressStart = 0;
    }
  }
}

void showHistory() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("(Mock) Yesterday in " + cityNames[city], 10, 10);
  //Needs another API to make this work
  tft.drawString("High: 12.3 C", 10, 30);
  tft.drawString("Low: 5.8 C", 10, 50);
  tft.drawString("Hold both buttons to return", 10, 140);
  delay(5000);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connecting...", 10, 10);
    delay(500);
  }
  drawBootScreen();
}
//Main loop
void loop() {
  while (true) {
    String choice = showMenu();
    unsigned long holdStart = 0;
    while (true) {
      //Break if both buttons are being hold
      if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == LOW) {
        if (holdStart == 0) holdStart = millis();
        if (millis() - holdStart > 2000) break;
      } else {
        holdStart = 0;
      }

      if (choice == "Forecast") {
        WeatherData data;
        fetchForecast(data);
        drawForecast(data);
      } else if (choice == "Settings") {
        showSettings();
      } else if (choice == "History") {
        showHistory();
      }
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