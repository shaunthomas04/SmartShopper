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
    M5.Display.println("START=clear  SELECT=switch  X=zip  Y=rec  B=Broadcast");

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
    
    // Adjusted DIGIT_Y slightly to center the whole row on the screen better
    const int DIGIT_Y = 100; 
    const int SPACING = 48;

    for (int i = 0; i < 5; i++) {
        int x = START_X + i * SPACING;
        
        if (i == zipCursorPos) {
            M5.Display.fillRoundRect(x - 4, DIGIT_Y - 4, DIGIT_W, DIGIT_H, 6, TFT_BLUE);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
            
            // Carets centered horizontally relative to the digit
            M5.Display.setCursor(x + 10, DIGIT_Y - 16);
            M5.Display.print("^");
            M5.Display.setCursor(x + 10, DIGIT_Y + DIGIT_H + 2);
            M5.Display.print("v");
        } else {
            M5.Display.fillRoundRect(x - 4, DIGIT_Y - 4, DIGIT_W, DIGIT_H, 6, TFT_DARKGREY);
        }

        M5.Display.setTextSize(4);
        M5.Display.setTextColor(TFT_WHITE, i == zipCursorPos ? TFT_BLUE : TFT_DARKGREY);
        
        // Vertical centering logic:
        // Box is 50px high. Text size 4 is ~28px high.
        // (50 - 28) / 2 = 11 pixels of padding.
        M5.Display.setCursor(x, DIGIT_Y + 11); 
        M5.Display.print(zipCode[i]);
    }

    drawUserIdOverlay();
}

// ----------- Draw: Record Screen (Centered) -----------
void drawRecordScreen() {
    M5.Display.clear();

    // Header section
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Record Voice");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setCursor(10, 38);
    M5.Display.println("Button=record/stop  A=send  B=back");

    // --- Button Geometry ---
    const int btnW = 140;
    const int btnH = 140;
    const int btnR = 70; // High radius for a circular look
    
    // Calculate X and Y to center on a 320x240 screen
    const int btnX = (320 - btnW) / 2;
    const int btnY = 60 + (180 - btnH) / 2; // Offset slightly for the header

    // Draw Button (Pulse color if recording)
    uint16_t btnColor = isRecording ? TFT_MAROON : TFT_RED;
    M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, btnR, btnColor);

    // --- Text Centering ---
    const char* label = isRecording ? "STOP" : "REC"; 
    
    M5.Display.setTextSize(3); // Size 3 is roughly 18x24 pixels per char
    int textWidth = strlen(label) * 18;
    int textHeight = 24;

    int textX = btnX + (btnW - textWidth) / 2;
    int textY = btnY + (btnH - textHeight) / 2;

    M5.Display.setTextColor(TFT_WHITE, btnColor);
    M5.Display.setCursor(textX, textY);
    M5.Display.print(label);
    
    drawUserIdOverlay();
}

// ----------- Feedback flash -----------
void flashFeedback(uint16_t color) {
    M5.Display.fillScreen(color);
    delay(80);
}


void drawBleBroadcastScreen() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Broadcasting");
    M5.Display.println("BLE Server!");

    drawUserIdOverlay();
}