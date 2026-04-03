#include <M5Unified.h>
#include "Adafruit_seesaw.h"
#include <vector>

// ----------- Seesaw Gamepad (same as your BLE code) -----------
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
enum Screen { SHOPPING_LIST, SUGGESTIONS };
Screen currentScreen = SHOPPING_LIST;

// ----------- Data -----------
std::vector<String> shoppingList;
std::vector<String> suggestions = {
  "Milk", "Eggs", "Bread", "Chicken", "Rice",
  "Apples", "Bananas", "Coffee", "Cheese", "Yogurt"
};

int selectedIndex = 0;
int listScrollOffset = 0;        // for scrolling the shopping list view
const int MAX_VISIBLE = 9;

// ----------- Joystick helpers -----------
const int CENTER   = 512;
const int DEADZONE = 100;

bool joystickUp()   { return (1023 - ss.analogRead(15)) < CENTER - DEADZONE; }
bool joystickDown() { return (1023 - ss.analogRead(15)) > CENTER + DEADZONE; }

// Debounce joystick so one tilt = one move
unsigned long lastJoyMove = 0;
const int JOY_REPEAT_MS = 200;

bool joystickMoved(bool (*dirFn)()) {
    if (dirFn() && millis() - lastJoyMove > JOY_REPEAT_MS) {
        lastJoyMove = millis();
        return true;
    }
    return false;
}

// ----------- Button edge detection (same pattern as your BLE code) -----------
// Returns true on the falling edge (button just pressed)
bool buttonJustPressed(uint32_t buttons, uint32_t pin) {
    return !(buttons & (1UL << pin)) && (lastButtons & (1UL << pin));
}

// ----------- Draw Functions -----------
void drawShoppingList() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Shopping List");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("START=clear  SELECT=switch");
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

void drawSuggestions() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Add Item");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("A=add  SELECT=switch screen");
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

    // Keep trying — same approach as your working BLE code
    while (!ss.begin(0x50)) {
        Serial.println("seesaw not found, retrying...");
        M5.Display.setCursor(10, 40);
        M5.Display.println("Retrying...");
        delay(500);
    }
    Serial.println("seesaw started");

    // Version check from your BLE code — tells you if wrong firmware
    uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    Serial.print("Seesaw product version: ");
    Serial.println(version);

    ss.pinModeBulk(button_mask, INPUT_PULLUP);
    ss.setGPIOInterrupts(button_mask, 1);

    lastButtons = ss.digitalReadBulk(button_mask);

    drawSuggestions();
    currentScreen = SUGGESTIONS;
}

// ----------- Loop -----------
void loop() {
    M5.update();

    uint32_t buttons = ss.digitalReadBulk(button_mask);

    // -------- SELECT: switch screens --------
    if (buttonJustPressed(buttons, BUTTON_SELECT)) {
        if (currentScreen == SUGGESTIONS) {
            currentScreen = SHOPPING_LIST;
            listScrollOffset = 0;
            drawShoppingList();
        } else {
            currentScreen = SUGGESTIONS;
            drawSuggestions();
        }
    }

    // -------- Suggestions screen --------
    if (currentScreen == SUGGESTIONS) {

        // Joystick scroll
        if (joystickMoved(joystickUp)) {
            if (selectedIndex > 0) {
                selectedIndex--;
                drawSuggestions();
            }
        }
        if (joystickMoved(joystickDown)) {
            if (selectedIndex < (int)suggestions.size() - 1) {
                selectedIndex++;
                drawSuggestions();
            }
        }

        // A button: add item
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
                flashFeedback(TFT_RED);  // already in list
            }
            drawSuggestions();
        }
    }

    // -------- Shopping List screen --------
    if (currentScreen == SHOPPING_LIST) {

        // Scroll through long lists with joystick
        if (joystickMoved(joystickUp)) {
            if (listScrollOffset > 0) {
                listScrollOffset--;
                drawShoppingList();
            }
        }
        if (joystickMoved(joystickDown)) {
            if (listScrollOffset + MAX_VISIBLE < (int)shoppingList.size()) {
                listScrollOffset++;
                drawShoppingList();
            }
        }

        // START button: clear list
        if (buttonJustPressed(buttons, BUTTON_START)) {
            shoppingList.clear();
            listScrollOffset = 0;
            drawShoppingList();
        }
    }

    lastButtons = buttons;
    delay(10);
}