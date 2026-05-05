#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://192.168.1.193:8000/nowplaying"; // 본인 맥 IP 입력

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connecting to WiFi...", 10, 10, 2);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
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

            tft.fillScreen(TFT_BLACK);
            tft.drawString(title, 10, 40, 4);
            tft.drawString(artist, 10, 80, 2);
        }
        http.end();
    }
    delay(5000); // 5초마다 업데이트
}