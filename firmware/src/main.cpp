#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "env.h"

#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();
U8g2_for_TFT_eSPI u8f;

// 줄바꿈 처리를 위한 함수 (간이 구현)
void drawWrappedText(const char* text, int x, int y, int maxWidth, int lineHeight) {
    String str = String(text);
    int startIdx = 0;
    int currentY = y;
    
    while (startIdx < str.length()) {
        int charCount = 0;
        int currentWidth = 0;
        
        while (startIdx + charCount < str.length()) {
            unsigned char c = (unsigned char)str[startIdx + charCount];
            int charWidth;
            int step;
            
            if (c < 128) { // 영문/숫자
                charWidth = 9;
                step = 1;
            } else if ((c & 0xE0) == 0xC0) {
                charWidth = 16;
                step = 2;
            } else if ((c & 0xF0) == 0xE0) {
                charWidth = 16;
                step = 3;
            } else {
                charWidth = 16;
                step = 4;
            }
            
            if (currentWidth + charWidth > maxWidth) break;
            
            currentWidth += charWidth;
            charCount += step;
        }
        
        if (charCount == 0) break;
        
        u8f.setCursor(x, currentY);
        u8f.print(str.substring(startIdx, startIdx + charCount));
        
        startIdx += charCount;
        currentY += lineHeight;
        
        if (currentY > 230) break; 
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    u8f.begin(tft); 
    u8f.setFontMode(1); 
    u8f.setFontDirection(0);

    // korean2 폰트 적용
    u8f.setFont(u8g2_font_unifont_t_korean2); 
    u8f.setForegroundColor(TFT_WHITE);
    
    u8f.setCursor(10, 40);
    u8f.print("와이파이 연결 중...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    tft.fillScreen(TFT_BLACK);
    u8f.setCursor(10, 40);
    u8f.print("연결 성공!");
    delay(1000);
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
            
            // 노래 제목 (상단)
            u8f.setFont(u8g2_font_unifont_t_korean2);
            u8f.setForegroundColor(TFT_WHITE);
            drawWrappedText(title, 10, 50, 300, 30);
            
            // 아티스트 (하단 노란색, 줄바꿈 적용)
            u8f.setForegroundColor(TFT_YELLOW);
            drawWrappedText(artist, 10, 180, 300, 25);
        }
        http.end();
    }
    delay(5000);
}
