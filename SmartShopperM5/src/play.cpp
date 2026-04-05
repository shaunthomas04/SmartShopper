#include <M5Unified.h>
#include "Adafruit_seesaw.h"
#include <vector>

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

// ----------- Record State -----------
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
        return;
    }

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
}

// ----------- Draw: Record Screen -----------
void drawRecordScreen() {
    M5.Display.clear();

    // Title
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Record");

    // Instructions
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 38);
    M5.Display.println("A or tap button = toggle  B = go back");

    // Always draw the red button
    M5.Display.fillRoundRect(REC_BTN_X, REC_BTN_Y, REC_BTN_W, REC_BTN_H, REC_BTN_R, TFT_RED);

    // Center label inside the button — textSize 3 = 18px wide, 24px tall per char
    const char* label = isRecording ? "STOP" : "RECORD";
    const int charW   = 18;
    const int charH   = 24;
    int textW  = strlen(label) * charW;
    int textX  = REC_BTN_X + (REC_BTN_W - textW) / 2;
    int textY  = REC_BTN_Y + (REC_BTN_H - charH) / 2;

    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
    M5.Display.setCursor(textX, textY);
    M5.Display.print(label);

    // REC indicator below button — only when recording
    if (isRecording) {
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        // Center "● REC" below button
        const char* recLabel = "* REC";
        int recW = strlen(recLabel) * 6;
        M5.Display.setCursor((320 - recW) / 2, REC_BTN_Y + REC_BTN_H + 16);
        M5.Display.print(recLabel);
    }
}

// ----------- Toggle recording -----------
void toggleRecording() {
    isRecording = !isRecording;
    Serial.println(isRecording ? "Recording started" : "Recording stopped");
    drawRecordScreen();
}

// ----------- Feedback flash -----------
void flashFeedback(uint16_t color) {
    M5.Display.fillScreen(color);
    delay(80);
}

// ----------- Setup -----------
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Starting seesaw...");

    while (!ss.begin(0x50)) {
        Serial.println("seesaw not found, retrying...");
        M5.Display.setCursor(10, 40);
        M5.Display.println("Retrying...");
        delay(500);
    }
    Serial.println("seesaw started");

    uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    Serial.print("Seesaw product version: ");
    Serial.println(version);

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

    // -------- X: open ZIP editor --------
    if (buttonJustPressed(buttons, BUTTON_X) && currentScreen != ZIP_EDITOR) {
        zipCursorPos  = 0;
        currentScreen = ZIP_EDITOR;
        drawZipEditor();
    }
    // -------- Y: open Record screen --------
    else if (buttonJustPressed(buttons, BUTTON_Y) && currentScreen != RECORD_SCREEN) {
        currentScreen = RECORD_SCREEN;
        drawRecordScreen();
    }
    // -------- SELECT: toggle Shopping List / Suggestions --------
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

        if (joystickMoved(joystickLeft)) {
            if (zipCursorPos > 0) { zipCursorPos--; drawZipEditor(); }
        }
        if (joystickMoved(joystickRight)) {
            if (zipCursorPos < 4) { zipCursorPos++; drawZipEditor(); }
        }
        if (joystickMoved(joystickUp)) {
            zipCode[zipCursorPos] = (zipCode[zipCursorPos] - '0' + 1) % 10 + '0';
            drawZipEditor();
        }
        if (joystickMoved(joystickDown)) {
            zipCode[zipCursorPos] = (zipCode[zipCursorPos] - '0' + 9) % 10 + '0';
            drawZipEditor();
        }

        if (buttonJustPressed(buttons, BUTTON_B)) {
            Serial.print("ZIP saved: ");
            Serial.println(zipCode);
            flashFeedback(TFT_GREEN);
            currentScreen    = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    // ======== RECORD SCREEN ========
    else if (currentScreen == RECORD_SCREEN) {

        // A button toggles
        if (buttonJustPressed(buttons, BUTTON_A)) {
            toggleRecording();
        }

        // Touch: tap inside the button to toggle
        if (M5.Touch.getCount() > 0) {
            auto t = M5.Touch.getDetail(0);
            if (t.wasPressed()) {
                if (t.x >= REC_BTN_X && t.x <= REC_BTN_X + REC_BTN_W &&
                    t.y >= REC_BTN_Y && t.y <= REC_BTN_Y + REC_BTN_H) {
                    toggleRecording();
                }
            }
        }

        // B: exit, auto-stop if recording
        if (buttonJustPressed(buttons, BUTTON_B)) {
            if (isRecording) {
                isRecording = false;
                Serial.println("Recording stopped (exited)");
            }
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
            String item = suggestions[selectedIndex];
            bool alreadyAdded = false;
            for (auto& s : shoppingList) {
                if (s == item) { alreadyAdded = true; break; }
            }
            if (!alreadyAdded) {
                shoppingList.push_back(item);
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
