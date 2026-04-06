#include "globals.h"
#include "config.h"
#include "types.h"
#include <vector>

// ----------- User ID -----------
const String USER_ID = USER_ID_STR;

// ----------- Seesaw -----------
uint32_t button_mask =
    (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
    (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

uint32_t lastButtons = 0;

// ----------- App State -----------
Screen currentScreen = SHOPPING_LIST;

// ----------- Suggestions (default hardcoded data) -----------
std::vector<Suggestion> suggestions = {
    {
        "Mainstays Super Soft Plush Blanket",
        "https://example.com/blanket1",
        "https://example.com/images/blanket1.jpg",
        "$21.64", 21.64, 4.5, 3500, "Walmart", "Nearby, 15 mi"
    },
    {
        "Less Stress Package Unwind & Recharge",
        "https://example.com/relaxation1",
        "https://example.com/images/relaxation1.jpg",
        "$89.99", 89.99, 4.8, 144, "Spoonful of Comfort", "Online"
    },
    {
        "Celestial Seasonings Everyday Wellness Tea Pack",
        "https://example.com/tea1",
        "https://example.com/images/tea1.jpg",
        "$5.49", 5.49, 4.8, 75, "Target", "Nearby, 5 mi"
    },
    {
        "FlexWorks Shiatsu Pillow Massager",
        "https://example.com/massager1",
        "https://example.com/images/massager1.jpg",
        "$19.88", 19.88, 3.5, 144, "Walmart", "Nearby, 12 mi"
    },
    {
        "Threshold Essential Oil Diffuser",
        "https://example.com/diffuser1",
        "https://example.com/images/diffuser1.jpg",
        "$20.00", 20.00, 2.4, 79, "Target", "Nearby, 5 mi"
    }
};

std::vector<Suggestion> shoppingList;

int selectedIndex    = 0;
int listScrollOffset = 0;

// ----------- ZIP Code -----------
char zipCode[6]   = "95762";
int  zipCursorPos = 0;

// ----------- Record Screen -----------
bool isRecording = false;

// ----------- Joystick timing -----------
unsigned long lastJoyMove = 0;
