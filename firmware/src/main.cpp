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
            if (c < 128) { charWidth = 8; step = 1; } // 좁은 영역에 맞춰 폰트폭 조정
            else { charWidth = 16; step = 3; }
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
    tft.invertDisplay(true); 
    tft.fillScreen(TFT_BLACK);
    u8f.begin(tft); 
    u8f.setFontMode(1); 
    TJpgDec.setCallback(tft_output);
    TJpgDec.setSwapBytes(true);
    String base = String(serverUrl);
    artworkUrl = base.substring(0, base.lastIndexOf("/")) + "/artwork";
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
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
                    size_t s = stream->available();
                    if (s) read += stream->readBytes(&buff[read], s);
                }
                // 왼쪽 (0, 20)에 200x200 아트워크 배치
                TJpgDec.drawJpg(0, 20, buff, len);
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
                if (hasArtwork) fetchAndDrawArtwork();
                
                // 오른쪽 영역 (210 ~ 320) 정보 표시
                u8f.setFont(u8g2_font_unifont_t_korean2);
                u8f.setForegroundColor(TFT_WHITE);
                // 제목 출력
                drawWrappedText(title.c_str(), 210, 40, 105, 25);
                
                // 아티스트 출력 (구분선 느낌으로 약간 띄움)
                u8f.setForegroundColor(TFT_YELLOW);
                u8f.setCursor(210, 180);
                drawWrappedText(artist.c_str(), 210, 180, 105, 20);
                
                lastTitle = title;
            }
        }
        http.end();
    }
    delay(5000);
}
