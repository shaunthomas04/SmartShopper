#include <M5Unified.h>
#include "Adafruit_seesaw.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <vector>

// ----------- WiFi & User -----------
const char* WIFI_SSID     = "CBU-LANCERS";
const char* WIFI_PASSWORD = "L@ncerN@tion";
const String USER_ID      = "shaun2026";

// ----------- Endpoint -----------
const String ENDPOINT = "https://smart-shopper-967923575473.europe-west1.run.app";

// ----------- Hardcoded WAV file on SD -----------
const char* WAV_PATH = "/record.wav";

// ----------- Seesaw Gamepad -----------
Adafruit_seesaw ss;

#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START     16

uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

uint32_t lastButtons = 0;

// ----------- App State -----------
enum Screen { SHOPPING_LIST, SUGGESTIONS, ZIP_EDITOR, RECORD_SCREEN };
Screen currentScreen = SHOPPING_LIST;

// ----------- Data -----------
std::vector<String> shoppingList;
std::vector<String> suggestions = {
  "Milk", "Eggs", "Bread", "Chicken", "Rice",
  "Apples", "Bananas", "Coffee", "Cheese", "Yogurt"
};

int selectedIndex    = 0;
int listScrollOffset = 0;
const int MAX_VISIBLE = 9;

// ----------- ZIP Code -----------
char zipCode[6]   = "95762";
int  zipCursorPos = 0;

// ----------- Record Screen State -----------
// We no longer actually record — just track whether user has "pressed" the button
// so the UI still shows RECORD/STOP toggle, and A sends the SD file.
bool isRecording = false;

// ----------- Record button bounds -----------
const int REC_BTN_X = 60;
const int REC_BTN_Y = 80;
const int REC_BTN_W = 200;
const int REC_BTN_H = 80;
const int REC_BTN_R = 12;

// ----------- Joystick helpers -----------
const int CENTER   = 512;
const int DEADZONE = 100;

bool joystickUp()    { return (1023 - ss.analogRead(15)) > CENTER + DEADZONE; }
bool joystickDown()  { return (1023 - ss.analogRead(15)) < CENTER - DEADZONE; }
bool joystickLeft()  { return ss.analogRead(14) > CENTER + DEADZONE; }
bool joystickRight() { return ss.analogRead(14) < CENTER - DEADZONE; }

unsigned long lastJoyMove = 0;
const int JOY_REPEAT_MS   = 180;

bool joystickMoved(bool (*dirFn)()) {
    if (dirFn() && millis() - lastJoyMove > JOY_REPEAT_MS) {
        lastJoyMove = millis();
        return true;
    }
    return false;
}

// ----------- Button edge detection -----------
bool buttonJustPressed(uint32_t buttons, uint32_t pin) {
    return !(buttons & (1UL << pin)) && (lastButtons & (1UL << pin));
}

// ----------- userId overlay (bottom-right corner) -----------
void drawUserIdOverlay() {
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    int w = USER_ID.length() * 6;
    M5.Display.setCursor(320 - w - 4, 240 - 10);
    M5.Display.print(USER_ID);
}

// ----------- Draw: Shopping List -----------
void drawShoppingList() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Shopping List");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("START=clear  SELECT=switch  X=zip  Y=rec");
    M5.Display.setTextSize(2);

    if (shoppingList.empty()) {
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.setCursor(10, 55);
        M5.Display.println("(Empty)");
    } else {
        int visible = min((int)shoppingList.size() - listScrollOffset, MAX_VISIBLE);
        for (int i = 0; i < visible; i++) {
            int idx = i + listScrollOffset;
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.setCursor(10, 50 + i * 20);
            M5.Display.println("- " + shoppingList[idx]);
        }
        if ((int)shoppingList.size() > MAX_VISIBLE) {
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
            M5.Display.setCursor(10, 50 + MAX_VISIBLE * 20);
            M5.Display.printf("(%d items, scroll with stick)", (int)shoppingList.size());
        }
    }
    drawUserIdOverlay();
}

// ----------- Draw: Suggestions -----------
void drawSuggestions() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Add Item");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("A=add  SELECT=switch  X=zip  Y=rec");
    M5.Display.setTextSize(2);

    for (int i = 0; i < (int)suggestions.size(); i++) {
        int y = 50 + i * 20;
        if (i == selectedIndex) {
            M5.Display.fillRect(0, y - 2, 320, 20, TFT_BLUE);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        M5.Display.setCursor(10, y);
        M5.Display.println(suggestions[i]);
    }
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    drawUserIdOverlay();
}

// ----------- Draw: ZIP Editor -----------
void drawZipEditor() {
    M5.Display.clear();

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Edit ZIP Code");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 38);
    M5.Display.println("UP/DOWN=change digit  LEFT/RIGHT=move  B=save");

    const int DIGIT_W  = 36;
    const int DIGIT_H  = 50;
    const int START_X  = 40;
    const int DIGIT_Y  = 80;
    const int SPACING  = 48;

    for (int i = 0; i < 5; i++) {
        int x = START_X + i * SPACING;
        if (i == zipCursorPos) {
            M5.Display.fillRoundRect(x - 4, DIGIT_Y - 4, DIGIT_W, DIGIT_H, 6, TFT_BLUE);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
            M5.Display.setCursor(x + 6, DIGIT_Y - 16);
            M5.Display.print("^");
            M5.Display.setCursor(x + 6, DIGIT_Y + DIGIT_H);
            M5.Display.print("v");
        } else {
            M5.Display.fillRoundRect(x - 4, DIGIT_Y - 4, DIGIT_W, DIGIT_H, 6, TFT_DARKGREY);
        }
        M5.Display.setTextSize(4);
        M5.Display.setTextColor(TFT_WHITE, i == zipCursorPos ? TFT_BLUE : TFT_DARKGREY);
        M5.Display.setCursor(x, DIGIT_Y + 8);
        M5.Display.print(zipCode[i]);
    }

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setCursor(10, 160);
    M5.Display.print("ZIP: ");
    M5.Display.println(zipCode);

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.setCursor(10, 190);
    M5.Display.println("B = Save & go back");

    drawUserIdOverlay();
}

// ----------- Draw: Record Screen -----------
void drawRecordScreen() {
    M5.Display.clear();

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Record");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 38);
    M5.Display.println("Tap button=toggle  A=send  B=back");

    // Always draw the red button
    M5.Display.fillRoundRect(REC_BTN_X, REC_BTN_Y, REC_BTN_W, REC_BTN_H, REC_BTN_R, TFT_RED);

    // Centered label swaps between RECORD / STOP
    const char* label = isRecording ? "STOP" : "RECORD";
    const int charW = 18;
    const int charH = 24;
    int textW = strlen(label) * charW;
    int textX = REC_BTN_X + (REC_BTN_W - textW) / 2;
    int textY = REC_BTN_Y + (REC_BTN_H - charH) / 2;

    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
    M5.Display.setCursor(textX, textY);
    M5.Display.print(label);

    // Status line below button
    M5.Display.setTextSize(1);
    if (isRecording) {
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(REC_BTN_X + 30, REC_BTN_Y + REC_BTN_H + 10);
        M5.Display.print("* RECORDING");
    } else {
        M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Display.setCursor(REC_BTN_X + 10, REC_BTN_Y + REC_BTN_H + 10);
        M5.Display.print("A = Send record.wav");
    }

    // ZIP reminder
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setCursor(10, 195);
    M5.Display.printf("ZIP: %s", zipCode);

    drawUserIdOverlay();
}

// ----------- Send /record.wav from SD via HTTP POST -----------
void sendRecording() {
    // Show sending screen
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

    // Open WAV file from SD
    File wavFile = SD.open(WAV_PATH, FILE_READ);
    if (!wavFile) {
        Serial.println("[ERROR] Could not open /record.wav from SD card");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 100);
        M5.Display.println("SD file not found!");
        drawUserIdOverlay();
        delay(2000);
        drawRecordScreen();
        return;
    }

    size_t fileSize = wavFile.size();
    Serial.printf("[INFO] Opened %s — %d bytes\n", WAV_PATH, fileSize);

    // Read entire file into RAM buffer
    uint8_t* wavBuf = (uint8_t*)malloc(fileSize);
    if (!wavBuf) {
        Serial.println("[ERROR] Not enough RAM to buffer WAV file");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 100);
        M5.Display.println("Out of memory!");
        wavFile.close();
        drawUserIdOverlay();
        delay(2000);
        drawRecordScreen();
        return;
    }
    wavFile.read(wavBuf, fileSize);
    wavFile.close();

    // Connect WiFi if needed
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[INFO] WiFi not connected, reconnecting...");
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.setCursor(10, 100);
        M5.Display.println("Connecting WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 30) {
            delay(500);
            tries++;
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi connection failed");
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 120);
        M5.Display.println("WiFi failed!");
        free(wavBuf);
        drawUserIdOverlay();
        delay(2000);
        drawRecordScreen();
        return;
    }

    // Build URL
    String url = ENDPOINT + "?zipCode=" + String(zipCode) + "&userId=" + USER_ID;
    Serial.printf("[INFO] POST to: %s\n", url.c_str());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "audio/wav");
    http.addHeader("X-User-Id", USER_ID);
    http.setTimeout(15000);  // 15s timeout for large files

    int httpCode = http.POST(wavBuf, fileSize);
    free(wavBuf);

    String responseBody = http.getString();

    // Serial output
    Serial.println("========== HTTP RESPONSE ==========");
    Serial.printf("HTTP Code : %d\n", httpCode);
    Serial.printf("Response  : %s\n", responseBody.c_str());
    Serial.println("===================================");

    http.end();

    // Show result on screen
    M5.Display.clear();
    M5.Display.setTextSize(2);
    if (httpCode > 0) {
        M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Display.setCursor(10, 20);
        M5.Display.printf("HTTP %d OK", httpCode);
    } else {
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setCursor(10, 20);
        M5.Display.printf("Error %d", httpCode);
    }

    // Show first ~200 chars of response body on screen
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 55);
    M5.Display.println("Response:");
    M5.Display.setCursor(10, 68);
    // Wrap long responses so they fit on screen
    String preview = responseBody.substring(0, 220);
    M5.Display.println(preview);

    drawUserIdOverlay();
    delay(3000);
    drawRecordScreen();
}

// ----------- Feedback flash -----------
void flashFeedback(uint16_t color) {
    M5.Display.fillScreen(color);
    delay(80);
}

#define SD_CS 4
// ----------- Setup -----------
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    Serial.println("Testing SD...");

    if (!SD.begin(SD_CS, SPI, 1000000)) {
        Serial.println("SD FAILED");
    } else {
        Serial.println("SD OK");

        if (SD.exists("/record.wav")) {
            Serial.println("File exists!");
        } else {
            Serial.println("File missing");
        }
    }

    // Start WiFi
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

// ----------- Loop -----------
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

        // Tap the red button to toggle RECORD / STOP label
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

        // A: send record.wav from SD
        if (buttonJustPressed(buttons, BUTTON_A)) {
            isRecording = false;  // reset toggle state
            sendRecording();
        }

        // B: go back
        if (buttonJustPressed(buttons, BUTTON_B)) {
            isRecording   = false;
            currentScreen = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    // ======== SUGGESTIONS ========
    else if (currentScreen == SUGGESTIONS) {

        if (joystickMoved(joystickUp))   { if (selectedIndex > 0) { selectedIndex--; drawSuggestions(); } }
        if (joystickMoved(joystickDown)) { if (selectedIndex < (int)suggestions.size() - 1) { selectedIndex++; drawSuggestions(); } }

        if (buttonJustPressed(buttons, BUTTON_A)) {
            String item = suggestions[selectedIndex];
            bool alreadyAdded = false;
            for (auto& s : shoppingList) { if (s == item) { alreadyAdded = true; break; } }
            if (!alreadyAdded) { shoppingList.push_back(item); flashFeedback(TFT_GREEN); }
            else               { flashFeedback(TFT_RED); }
            drawSuggestions();
        }
    }

    // ======== SHOPPING LIST ========
    else if (currentScreen == SHOPPING_LIST) {

        if (joystickMoved(joystickUp))   { if (listScrollOffset > 0) { listScrollOffset--; drawShoppingList(); } }
        if (joystickMoved(joystickDown)) { if (listScrollOffset + MAX_VISIBLE < (int)shoppingList.size()) { listScrollOffset++; drawShoppingList(); } }

        if (buttonJustPressed(buttons, BUTTON_START)) {
            shoppingList.clear();
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    lastButtons = buttons;
    delay(10);
}
