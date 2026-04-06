#include "ui.h"
#include "config.h"
#include "globals.h"
#include <M5Unified.h>

// ----------- userId overlay (bottom-right corner) -----------
void drawUserIdOverlay() {
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    int w = USER_ID.length() * 6;
    M5.Display.setCursor(320 - w - 4, 240 - 10);
    M5.Display.print(USER_ID);
}

// ----------- Draw: Shopping List (with Scrolling & Highlighting) -----------
void drawShoppingList() {
    M5.Display.clear();

    // Header section
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Shopping List");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("START=clear  SELECT=switch  X=zip  Y=rec");

    const int itemSpacing = 18;
    const int startY      = 50;
    const int maxVisible  = 10; // Number of items that fit on screen

    if (shoppingList.empty()) {
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.setCursor(10, startY);
        M5.Display.println("(Empty)");
    } else {
        // --- Scrolling Logic ---
        static int listTopIndex = 0; 
        
        // Adjust the "window" based on selectedIndex
        if (selectedIndex >= listTopIndex + maxVisible) {
            listTopIndex = selectedIndex - maxVisible + 1;
        }
        if (selectedIndex < listTopIndex) {
            listTopIndex = selectedIndex;
        }

        int endIndex = min((int)shoppingList.size(), listTopIndex + maxVisible);

        for (int i = listTopIndex; i < endIndex; i++) {
            // Calculate Y relative to the scroll position
            int y = startY + (i - listTopIndex) * itemSpacing;

            if (i == selectedIndex) {
                // Blue selection bar
                M5.Display.fillRect(0, y - 2, 320, itemSpacing, TFT_BLUE);
                M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
            } else {
                M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            }

            M5.Display.setTextSize(1);
            M5.Display.setCursor(10, y);
            M5.Display.printf("%s - %s",
                shoppingList[i].price.c_str(),
                shoppingList[i].name.c_str());
        }
    }

    drawUserIdOverlay();
}

// ----------- Draw: Suggestions (with Scrolling) -----------
void drawSuggestions() {
    M5.Display.clear();

    // Header section
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Add Item");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 34);
    M5.Display.println("A=add  SELECT=switch  X=zip  Y=rec");

    const int itemSpacing = 18;
    const int startY      = 50;
    const int maxVisible  = 10; // Adjust this based on your screen height

    // --- Scrolling Logic ---
    static int topIndex = 0; // Tracks which item is at the top of the list
    
    // If selection goes below the visible window, scroll down
    if (selectedIndex >= topIndex + maxVisible) {
        topIndex = selectedIndex - maxVisible + 1;
    }
    // If selection goes above the visible window, scroll up
    if (selectedIndex < topIndex) {
        topIndex = selectedIndex;
    }

    // Determine how many items we can actually draw
    int endIndex = min((int)suggestions.size(), topIndex + maxVisible);

    for (int i = topIndex; i < endIndex; i++) {
        // Calculate Y relative to the topIndex
        int y = startY + (i - topIndex) * itemSpacing;

        if (i == selectedIndex) {
            // Highlight bar
            M5.Display.fillRect(0, y - 2, 320, itemSpacing, TFT_BLUE);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }

        M5.Display.setTextSize(1);
        M5.Display.setCursor(10, y);
        M5.Display.printf("%s - %s",
            suggestions[i].price.c_str(),
            suggestions[i].name.c_str());
    }

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

    const int DIGIT_W = 36;
    const int DIGIT_H = 50;
    const int START_X = 40;
    const int DIGIT_Y = 80;
    const int SPACING = 48;

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

    M5.Display.fillRoundRect(REC_BTN_X, REC_BTN_Y, REC_BTN_W, REC_BTN_H, REC_BTN_R, TFT_RED);

    const char* label = isRecording ? "STOP" : "RECORD";
    const int charW   = 18;
    const int charH   = 24;
    int textW = strlen(label) * charW;
    int textX = REC_BTN_X + (REC_BTN_W - textW) / 2;
    int textY = REC_BTN_Y + (REC_BTN_H - charH) / 2;

    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
    M5.Display.setCursor(textX, textY);
    M5.Display.print(label);

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

    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setCursor(10, 195);
    M5.Display.printf("ZIP: %s", zipCode);

    drawUserIdOverlay();
}

// ----------- Feedback flash -----------
void flashFeedback(uint16_t color) {
    M5.Display.fillScreen(color);
    delay(80);
}
