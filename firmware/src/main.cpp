#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "env.h"

#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();
U8g2_for_TFT_eSPI u8f;

String lastTitle = "";
String artworkUrl = "";

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void drawWrappedText(const char* text, int x, int y, int maxWidth, int lineHeight) {
    String str = String(text);
    int startIdx = 0;
    int currentY = y;
    while (startIdx < str.length()) {
        int charCount = 0;
        int currentWidth = 0;
        while (startIdx + charCount < str.length()) {
            unsigned char c = (unsigned char)str[startIdx + charCount];
            int charWidth, step;
            if (c < 128) { charWidth = 9; step = 1; }
            else if ((c & 0xE0) == 0xC0) { charWidth = 16; step = 2; }
            else if ((c & 0xF0) == 0xE0) { charWidth = 16; step = 3; }
            else { charWidth = 16; step = 4; }
            if (currentWidth + charWidth > maxWidth) break;
            currentWidth += charWidth;
            charCount += step;
        }
        if (charCount == 0) break;
        u8f.setCursor(x, currentY);
        u8f.print(str.substring(startIdx, startIdx + charCount));
        startIdx += charCount;
        currentY += lineHeight;
        if (currentY > 235) break; 
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 

    tft.init();
    tft.setRotation(1); 
    
    // 색상 반전 해결 (필요 시 true/false 변경)
    tft.invertDisplay(true); 
    
    tft.fillScreen(TFT_BLACK);

    u8f.begin(tft); 
    u8f.setFontMode(1); 
    u8f.setFont(u8g2_font_unifont_t_korean2); 
    u8f.setForegroundColor(TFT_WHITE);
    
    TJpgDec.setCallback(tft_output);
    TJpgDec.setSwapBytes(true);

    String base = String(serverUrl);
    artworkUrl = base.substring(0, base.lastIndexOf("/")) + "/artwork";

    u8f.setCursor(10, 40);
    u8f.print("와이파이 연결 중...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    tft.fillScreen(TFT_BLACK);
}

void fetchAndDrawArtwork() {
    HTTPClient http;
    http.begin(artworkUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len > 0) {
            uint8_t* buff = (uint8_t*)malloc(len);
            if (buff) {
                WiFiClient* stream = http.getStreamPtr();
                int read = 0;
                while (http.connected() && read < len) {
                    size_t size = stream->available();
                    if (size) {
                        int r = stream->readBytes(&buff[read], size);
                        read += r;
                    }
                }
                // 320x240 화면에서 240x240 아트를 가로 중앙(x=40)에 배치
                TJpgDec.drawJpg(40, 0, buff, len);
                free(buff);
            }
        }
    }
    http.end();
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

            String title = doc["title"];
            String artist = doc["artist"];
            bool hasArtwork = doc["has_artwork"];

            if (title != lastTitle) {
                tft.fillScreen(TFT_BLACK);
                if (hasArtwork) {
                    fetchAndDrawArtwork();
                }
                
                // 정보 출력 영역 (아래쪽 겹치지 않게 하단 오버레이)
                // 240x240 아트가 0~240 높이를 차지하므로, 
                // 하단 60픽셀 정도에 반투명 느낌으로 검은색 바를 깔고 정보 출력
                tft.fillRect(0, 180, 320, 60, TFT_BLACK); 
                u8f.setFont(u8g2_font_unifont_t_korean2);
                u8f.setForegroundColor(TFT_WHITE);
                drawWrappedText(title.c_str(), 10, 200, 300, 20);
                u8f.setForegroundColor(TFT_YELLOW);
                u8f.setCursor(10, 235);
                u8f.print(artist);
                
                lastTitle = title;
            }
        }
        http.end();
    }
    delay(5000);
}
