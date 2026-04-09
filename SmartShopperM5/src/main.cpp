#include <M5Unified.h>
#include <WiFi.h>
#include <SD.h>

#include "config.h"
#include "types.h"
#include "globals.h"
#include "input.h"
#include "ui.h"
#include "network.h"
#include "ble.h"

// -------- Recording constants --------
static constexpr size_t RECORD_SAMPLE_RATE = 16000;
static constexpr size_t RECORD_LENGTH      = 200;   // samples per chunk
static constexpr size_t RECORD_CHUNKS      = 256;   // max chunks before auto-stop
static constexpr size_t RECORD_SIZE        = RECORD_CHUNKS * RECORD_LENGTH;

// -------- Recording state --------
static int16_t* rec_data   = nullptr;
static size_t   rec_chunks = 0;
static bool     micActive  = false;

// -------- WAV header writer --------
static void writeWavHeader(File& file, uint32_t dataSize) {
    uint32_t sampleRate    = RECORD_SAMPLE_RATE;
    uint16_t channels      = 1;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate      = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign    = channels * bitsPerSample / 8;
    uint32_t fmtSize       = 16;
    uint16_t fmtCode       = 1; // PCM
    uint32_t fileSize      = dataSize + 36;

    file.write((const uint8_t*)"RIFF", 4);
    file.write((const uint8_t*)&fileSize,      4);
    file.write((const uint8_t*)"WAVE",         4);
    file.write((const uint8_t*)"fmt ",         4);
    file.write((const uint8_t*)&fmtSize,       4);
    file.write((const uint8_t*)&fmtCode,       2);
    file.write((const uint8_t*)&channels,      2);
    file.write((const uint8_t*)&sampleRate,    4);
    file.write((const uint8_t*)&byteRate,      4);
    file.write((const uint8_t*)&blockAlign,    2);
    file.write((const uint8_t*)&bitsPerSample, 2);
    file.write((const uint8_t*)"data",         4);
    file.write((const uint8_t*)&dataSize,      4);
}

// -------- Start mic + allocate buffer --------
static void startRecording() {
    if (micActive) return;

    rec_data = (int16_t*)heap_caps_malloc(RECORD_SIZE * sizeof(int16_t), MALLOC_CAP_8BIT);
    if (!rec_data) {
        Serial.println("[ERROR] Failed to allocate recording buffer");
        return;
    }
    memset(rec_data, 0, RECORD_SIZE * sizeof(int16_t));
    rec_chunks = 0;

    M5.Speaker.end();
    M5.Mic.begin();
    micActive   = true;
    isRecording = true;
    Serial.println("[INFO] Recording started");
    drawRecordScreen();
}

// -------- Stop mic, save to SD, free buffer --------
static void stopAndSaveRecording() {
    if (!micActive) return;

    while (M5.Mic.isRecording()) { M5.delay(1); }
    M5.Mic.end();
    micActive   = false;
    isRecording = false;
    Serial.printf("[INFO] Recording stopped — %u chunks captured\n", rec_chunks);

    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Saving...");

    if (!rec_data || rec_chunks == 0) {
        Serial.println("[WARN] Nothing recorded, skipping save");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 40);
        M5.Display.println("Nothing to save!");
        if (rec_data) { free(rec_data); rec_data = nullptr; }
        delay(1500);
        drawRecordScreen();
        return;
    }

    if (SD.exists(WAV_PATH)) SD.remove(WAV_PATH);
    File file = SD.open(WAV_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("[ERROR] Could not open WAV_PATH for writing");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 40);
        M5.Display.println("SD write failed!");
    } else {
        uint32_t sampleCount = rec_chunks * RECORD_LENGTH;
        uint32_t dataBytes   = sampleCount * sizeof(int16_t);
        writeWavHeader(file, dataBytes);
        file.write((const uint8_t*)rec_data, dataBytes);
        file.close();
        Serial.printf("[INFO] Saved %u bytes to %s\n", dataBytes + 44, WAV_PATH);
        M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Display.setCursor(10, 40);
        M5.Display.println("Saved!");
    }

    free(rec_data);
    rec_data = nullptr;

    delay(1000);
    drawRecordScreen();
}

// -------- Capture one chunk into buffer (called each loop tick) --------
static void tickRecording() {
    if (!micActive || !rec_data) return;
    if (rec_chunks >= RECORD_CHUNKS) {
        Serial.println("[INFO] Buffer full, auto-stopping");
        stopAndSaveRecording();
        return;
    }

    int16_t* chunk = &rec_data[rec_chunks * RECORD_LENGTH];
    if (M5.Mic.record(chunk, RECORD_LENGTH, RECORD_SAMPLE_RATE)) {
        rec_chunks++;
    }
}

// =====================================================================

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

    Serial.println("[INFO] WiFi will connect on demand.");

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

    tickRecording();

    uint32_t buttons = ss.digitalReadBulk(button_mask);

    // -------- Global nav — shut down BLE if leaving BLE_BROADCAST --------
    if (currentScreen == BLE_BROADCAST) {
        if (buttonJustPressed(buttons, BUTTON_X) ||
            buttonJustPressed(buttons, BUTTON_Y)) {
            bleStop();
        }
    }

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

        if (!isRecording && M5.Touch.getCount() > 0) {
            auto t = M5.Touch.getDetail(0);
            if (t.wasPressed()) {
                if (t.x >= REC_BTN_X && t.x <= REC_BTN_X + REC_BTN_W &&
                    t.y >= REC_BTN_Y && t.y <= REC_BTN_Y + REC_BTN_H) {
                    startRecording();
                }
            }
        }

        if (isRecording && buttonJustPressed(buttons, BUTTON_START)) {
            stopAndSaveRecording();
        }

        if (!isRecording && buttonJustPressed(buttons, BUTTON_A)) {
            sendRecording();
        }

        if (buttonJustPressed(buttons, BUTTON_B)) {
            if (micActive) {
                while (M5.Mic.isRecording()) { M5.delay(1); }
                M5.Mic.end();
                micActive = false;
                if (rec_data) { free(rec_data); rec_data = nullptr; }
            }
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
            if (selectedIndex > 0) { selectedIndex--; drawShoppingList(); }
        }
        if (joystickMoved(joystickDown)) {
            if (selectedIndex < (int)shoppingList.size() - 1) { selectedIndex++; drawShoppingList(); }
        }
        if (buttonJustPressed(buttons, BUTTON_START)) {
            shoppingList.clear();
            selectedIndex = 0;
            drawShoppingList();
        }

        if (buttonJustPressed(buttons, BUTTON_B)) {
            bleSetup();
            bleNotifyShoppingList();
            currentScreen = BLE_BROADCAST;
            drawBleBroadcastScreen();
        }
    }


    // ======== BLE BROADCAST ========
    // (add this block inside the else-if chain, after SHOPPING_LIST)
    else if (currentScreen == BLE_BROADCAST) {
        if (!shoppingList.empty()) {
            if (joystickMoved(joystickUp)) {
                if (selectedIndex > 0) {
                    selectedIndex--;
                    Serial.printf("[BLE] Selected: %s - %s\n",
                        shoppingList[selectedIndex].price.c_str(),
                        shoppingList[selectedIndex].name.c_str());
                    drawBleBroadcastScreen();
                }
            }
            if (joystickMoved(joystickDown)) {
                if (selectedIndex < (int)shoppingList.size() - 1) {
                    selectedIndex++;
                    Serial.printf("[BLE] Selected: %s - %s\n",
                        shoppingList[selectedIndex].price.c_str(),
                        shoppingList[selectedIndex].name.c_str());
                    drawBleBroadcastScreen();
                }
            }
        }
    }

    
    lastButtons = buttons;
    delay(10);
}