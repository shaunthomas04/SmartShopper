#include <M5Unified.h>
#include <WiFi.h>
#include <SD.h>

#include "config.h"
#include "types.h"
#include "globals.h"
#include "input.h"
#include "ui.h"
#include "network.h"

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    Serial.println("Testing SD...");
    if (!SD.begin(SD_CS, SPI, 1000000)) {
        Serial.println("SD FAILED");
    } else {
        Serial.println("SD OK");
        if (SD.exists(WAV_PATH)) {
            Serial.println("File exists!");
        } else {
            Serial.println("File missing");
        }
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("[INFO] WiFi connecting...");

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Starting seesaw...");

    while (!ss.begin(0x50)) {
        Serial.println("[WARN] seesaw not found, retrying...");
        M5.Display.setCursor(10, 40);
        M5.Display.println("Retrying...");
        delay(500);
    }
    Serial.println("[INFO] Seesaw started");

    uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    Serial.printf("[INFO] Seesaw product version: %d\n", version);

    ss.pinModeBulk(button_mask, INPUT_PULLUP);
    ss.setGPIOInterrupts(button_mask, 1);
    lastButtons = ss.digitalReadBulk(button_mask);

    drawShoppingList();
    currentScreen = SHOPPING_LIST;
}

void loop() {
    M5.update();

    uint32_t buttons = ss.digitalReadBulk(button_mask);

    // -------- Global nav --------
    if (buttonJustPressed(buttons, BUTTON_X) && currentScreen != ZIP_EDITOR) {
        zipCursorPos  = 0;
        currentScreen = ZIP_EDITOR;
        drawZipEditor();
    }
    else if (buttonJustPressed(buttons, BUTTON_Y) && currentScreen != RECORD_SCREEN) {
        currentScreen = RECORD_SCREEN;
        drawRecordScreen();
    }
    else if (buttonJustPressed(buttons, BUTTON_SELECT) &&
             currentScreen != ZIP_EDITOR && currentScreen != RECORD_SCREEN) {
        if (currentScreen == SUGGESTIONS) {
            currentScreen    = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        } else {
            currentScreen = SUGGESTIONS;
            drawSuggestions();
        }
    }

    // ======== ZIP EDITOR ========
    if (currentScreen == ZIP_EDITOR) {
        if (joystickMoved(joystickLeft))  { if (zipCursorPos > 0) { zipCursorPos--; drawZipEditor(); } }
        if (joystickMoved(joystickRight)) { if (zipCursorPos < 4) { zipCursorPos++; drawZipEditor(); } }
        if (joystickMoved(joystickUp))    { zipCode[zipCursorPos] = (zipCode[zipCursorPos] - '0' + 1) % 10 + '0'; drawZipEditor(); }
        if (joystickMoved(joystickDown))  { zipCode[zipCursorPos] = (zipCode[zipCursorPos] - '0' + 9) % 10 + '0'; drawZipEditor(); }

        if (buttonJustPressed(buttons, BUTTON_B)) {
            Serial.printf("[INFO] ZIP saved: %s\n", zipCode);
            flashFeedback(TFT_GREEN);
            currentScreen    = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    // ======== RECORD SCREEN ========
    else if (currentScreen == RECORD_SCREEN) {
        if (M5.Touch.getCount() > 0) {
            auto t = M5.Touch.getDetail(0);
            if (t.wasPressed()) {
                if (t.x >= REC_BTN_X && t.x <= REC_BTN_X + REC_BTN_W &&
                    t.y >= REC_BTN_Y && t.y <= REC_BTN_Y + REC_BTN_H) {
                    isRecording = !isRecording;
                    Serial.println(isRecording ? "[INFO] Button -> STOP" : "[INFO] Button -> RECORD");
                    drawRecordScreen();
                }
            }
        }

        if (buttonJustPressed(buttons, BUTTON_A)) {
            isRecording = false;
            sendRecording();
        }

        if (buttonJustPressed(buttons, BUTTON_B)) {
            isRecording      = false;
            currentScreen    = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    // ======== SUGGESTIONS ========
    else if (currentScreen == SUGGESTIONS) {
        if (joystickMoved(joystickUp)) {
            if (selectedIndex > 0) { selectedIndex--; drawSuggestions(); }
        }
        if (joystickMoved(joystickDown)) {
            if (selectedIndex < (int)suggestions.size() - 1) { selectedIndex++; drawSuggestions(); }
        }

        if (buttonJustPressed(buttons, BUTTON_A)) {
            const Suggestion& sel = suggestions[selectedIndex];
            bool alreadyAdded = false;
            for (auto& s : shoppingList) {
                if (s.name == sel.name) { alreadyAdded = true; break; }
            }
            if (!alreadyAdded) {
                shoppingList.push_back(sel);
                flashFeedback(TFT_GREEN);
            } else {
                flashFeedback(TFT_RED);
            }
            drawSuggestions();
        }
    }

    // ======== SHOPPING LIST ========
    else if (currentScreen == SHOPPING_LIST) {
        if (joystickMoved(joystickUp)) {
            if (listScrollOffset > 0) { listScrollOffset--; drawShoppingList(); }
        }
        if (joystickMoved(joystickDown)) {
            if (listScrollOffset + MAX_VISIBLE < (int)shoppingList.size()) { listScrollOffset++; drawShoppingList(); }
        }

        if (buttonJustPressed(buttons, BUTTON_START)) {
            shoppingList.clear();
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    lastButtons = buttons;
    delay(10);
}
