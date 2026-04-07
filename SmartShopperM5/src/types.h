#pragma once
#include <Arduino.h>

// ----------- Screen States -----------
enum Screen {
    SHOPPING_LIST,
    SUGGESTIONS,
    ZIP_EDITOR,
    RECORD_SCREEN,
    BLE_BROADCAST
};

// ----------- Suggestion Data -----------
struct Suggestion {
    String name;
    String link;
    String image;
    String price;
    float  cost;
    float  rating;
    int    reviews;
    String store;
    String distance;
};
