#pragma once
#include <Arduino.h>
#include "Adafruit_seesaw.h"

extern Adafruit_seesaw ss;

bool joystickUp();
bool joystickDown();
bool joystickLeft();
bool joystickRight();

// Returns true if the direction function fires and repeat debounce has elapsed
bool joystickMoved(bool (*dirFn)());

// Returns true if pin was pressed last frame but released this frame (edge detect)
bool buttonJustPressed(uint32_t buttons, uint32_t pin);
