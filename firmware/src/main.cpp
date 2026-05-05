#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "env.h"

// CYD 백라이트 핀 번호
#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);

    // 백라이트 강제 켜기
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 

    tft.init();
    tft.setRotation(1);
    
    // 화면 테스트를 위해 처음에 빨간색으로 채움
    tft.fillScreen(TFT_RED); 
    delay(500);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Connecting to WiFi...", 10, 10, 2);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi Connected!");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WiFi Connected!", 10, 10, 2);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);

            const char* title = doc["title"];
            const char* artist = doc["artist"];

            Serial.printf("Update Display: %s - %s\n", title, artist);

            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(title, 10, 60, 4); 
            
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString(artist, 10, 100, 2);
        } else {
            Serial.printf("HTTP Error: %d\n", httpCode);
        }
        http.end();
    }
    delay(5000);
}
