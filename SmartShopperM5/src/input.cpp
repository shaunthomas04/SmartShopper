#include "input.h"
#include "config.h"
#include "globals.h"

Adafruit_seesaw ss;

bool joystickUp()    { return (1023 - ss.analogRead(15)) > JOY_CENTER + JOY_DEADZONE; }
bool joystickDown()  { return (1023 - ss.analogRead(15)) < JOY_CENTER - JOY_DEADZONE; }
bool joystickLeft()  { return ss.analogRead(14) > JOY_CENTER + JOY_DEADZONE; }
bool joystickRight() { return ss.analogRead(14) < JOY_CENTER - JOY_DEADZONE; }

bool joystickMoved(bool (*dirFn)()) {
    if (dirFn() && millis() - lastJoyMove > JOY_REPEAT_MS) {
        lastJoyMove = millis();
        return true;
    }
    return false;
}

bool buttonJustPressed(uint32_t buttons, uint32_t pin) {
    return !(buttons & (1UL << pin)) && (lastButtons & (1UL << pin));
}
