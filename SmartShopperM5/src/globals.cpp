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

    // {
    //     "Mainstays Super Soft Plush Blanket",
    //     "https://www.walmart.com/ip/Mainstays-Plush-Blanket/",
    //     "https://images.unsplash.com/photo-1581578731548-c64695cc6952",
    //     "$21.64", 21.64, 4.5, 3500, "Walmart", "Nearby, 15 mi"
    // },

    // {
    //     "Less Stress Package Unwind & Recharge",
    //     "https://www.spoonfulofcomfort.com/",
    //     "https://images.unsplash.com/photo-1600891964599-f61ba0e24092",
    //     "$89.99", 89.99, 4.8, 144, "Spoonful of Comfort", "Online"
    // },

    // {
    //     "Celestial Seasonings Everyday Wellness Tea Pack",
    //     "https://www.target.com/",
    //     "https://images.unsplash.com/photo-1509042239860-f550ce710b93",
    //     "$5.49", 5.49, 4.8, 75, "Target", "Nearby, 5 mi"
    // },

    // {
    //     "FlexWorks Shiatsu Pillow Massager",
    //     "https://www.walmart.com/",
    //     "https://images.unsplash.com/photo-1591343395902-1adcb454c8b1",
    //     "$19.88", 19.88, 3.5, 144, "Walmart", "Nearby, 12 mi"
    // },

    // {
    //     "Threshold Essential Oil Diffuser",
    //     "https://www.target.com/",
    //     "https://images.unsplash.com/photo-1608571423902-eed4a5ad8108",
    //     "$20.00", 20.00, 2.4, 79, "Target", "Nearby, 5 mi"
    // },

    // {
    //     "Aromatherapy Candle Set",
    //     "https://www.amazon.com/",
    //     "https://images.unsplash.com/photo-1603006905003-be475563bc59",
    //     "$18.99", 18.99, 4.6, 980, "Amazon", "Online"
    // },

    // {
    //     "Weighted Blanket 15 lb",
    //     "https://www.amazon.com/",
    //     "https://images.unsplash.com/photo-1616627452488-1d0e4b3d6f57",
    //     "$49.99", 49.99, 4.7, 2100, "Amazon", "Online"
    // },

    // {
    //     "Herbal Sleep Tea Variety Pack",
    //     "https://www.target.com/",
    //     "https://images.unsplash.com/photo-1511920170033-f8396924c348",
    //     "$12.99", 12.99, 4.5, 430, "Target", "Nearby, 5 mi"
    // },

    // {
    //     "Neck and Shoulder Massager",
    //     "https://www.walmart.com/",
    //     "https://images.unsplash.com/photo-1599058917765-a780eda07a3e",
    //     "$29.99", 29.99, 4.2, 870, "Walmart", "Nearby, 12 mi"
    // },

    // {
    //     "Ultrasonic Essential Oil Diffuser",
    //     "https://www.target.com/",
    //     "https://images.unsplash.com/photo-1583947215259-38e31be8751f",
    //     "$24.99", 24.99, 4.1, 320, "Target", "Nearby, 5 mi"
    // }
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
