#pragma once
#include <Arduino.h>
#include <vector>
#include "types.h"

// ----------- Seesaw -----------
extern uint32_t button_mask;
extern uint32_t lastButtons;

// ----------- App State -----------
extern Screen currentScreen;

// ----------- Data -----------
extern std::vector<Suggestion> shoppingList;
extern std::vector<Suggestion> suggestions;

extern int selectedIndex;
extern int listScrollOffset;

// ----------- ZIP Code -----------
extern char zipCode[6];
extern int  zipCursorPos;

// ----------- Record Screen State -----------
extern bool isRecording;

// ----------- Joystick timing -----------
extern unsigned long lastJoyMove;

// ----------- User ID (runtime String) -----------
extern const String USER_ID;
