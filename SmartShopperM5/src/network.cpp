#include "network.h"
#include "config.h"
#include "globals.h"
#include "ui.h"
#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <ArduinoJson.h>

// ----------- Parse server JSON response into suggestions -----------
void updateSuggestions(String serverResponse) {
    if (serverResponse == "INVALID") {
        Serial.print("Invalid server response");
        return;
    }

    const size_t jsonCapacity = 768 + 250;
    DynamicJsonDocument objResponse(jsonCapacity);

    DeserializationError error = deserializeJson(objResponse, serverResponse);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    std::vector<Suggestion> responseSuggestions;
    JsonArray shoppingResults = objResponse["shoppingResults"];

    for (JsonObject item : shoppingResults) {
        Suggestion s;
        s.name     = item["name"]     | "";
        s.link     = item["link"]     | "";
        s.image    = item["image"]    | "";
        s.price    = item["price"]    | "";
        s.cost     = item["cost"]     | 0.0f;
        s.rating   = item["rating"]   | 0.0f;
        s.reviews  = item["reviews"]  | 0;
        s.store    = item["store"]    | "";
        s.distance = item["distance"] | "";
        responseSuggestions.push_back(s);
    }

    suggestions = responseSuggestions;
}

// ----------- Send /record.wav from SD via HTTP POST -----------
void sendRecording() {
    // 1. UI: Sending screen
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Sending...");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 40);
    M5.Display.printf("File : %s", WAV_PATH);
    M5.Display.setCursor(10, 55);
    M5.Display.printf("ZIP  : %s", zipCode);
    M5.Display.setCursor(10, 70);
    M5.Display.printf("User : %s", USER_ID.c_str());
    drawUserIdOverlay();

    // 2. Open WAV file from SD
    File wavFile = SD.open(WAV_PATH, FILE_READ);
    if (!wavFile) {
        Serial.println("[ERROR] Could not open /record.wav");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 100);
        M5.Display.println("SD file not found!");
        delay(2000);
        drawRecordScreen();
        return;
    }

    size_t fileSize = wavFile.size();
    uint8_t* wavBuf = (uint8_t*)malloc(fileSize);
    if (!wavBuf) {
        Serial.println("[ERROR] Out of RAM");
        wavFile.close();
        return;
    }
    wavFile.read(wavBuf, fileSize);
    wavFile.close();

    // 3. WiFi check / reconnect
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 20) {
            delay(500);
            tries++;
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi Failed");
        free(wavBuf);
        return;
    }

    Serial.print("[INFO] Connected! IP: ");
    Serial.println(WiFi.localIP());

    // 4. HTTP POST
    String url = String(ENDPOINT) + "?zipCode=" + String(zipCode);
    Serial.printf("[INFO] Attempting POST to: %s\n", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "audio/wav");
        http.setTimeout(60000);

        int    httpCode    = http.POST(wavBuf, fileSize);
        String responseBody = "";

        if (httpCode > 0) {
            responseBody = http.getString();
            updateSuggestions(responseBody);
        } else {
            responseBody = "Error: " + String(http.errorToString(httpCode).c_str());
        }

        Serial.println("========== HTTP RESPONSE ==========");
        Serial.printf("HTTP Code : %d\n", httpCode);
        Serial.printf("Response  : %s\n", responseBody.c_str());
        Serial.println("===================================");

        // 5. UI: Result
        M5.Display.clear();
        M5.Display.setTextSize(2);
        if (httpCode == HTTP_CODE_OK || httpCode == 201) {
            M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5.Display.setCursor(10, 20);
            M5.Display.printf("SUCCESS (%d)", httpCode);
        } else {
            M5.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5.Display.setCursor(10, 20);
            M5.Display.printf("FAILED (%d)", httpCode);
        }

        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setCursor(10, 55);
        M5.Display.println("Response:");
        M5.Display.setCursor(10, 70);
        M5.Display.println(responseBody.substring(0, 200));

        http.end();
    } else {
        Serial.println("[ERROR] HTTP.begin failed - Check URL format");
    }

    // 6. Cleanup
    free(wavBuf);
    drawUserIdOverlay();
    delay(3000);
    drawRecordScreen();
}
