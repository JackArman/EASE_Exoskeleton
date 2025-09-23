#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <Arduino.h>

class Button {
public:
    Button(int pin)
        : pin(pin), lastState(LOW) {}

    void begin() {
        pinMode(pin, INPUT_PULLUP);
    }

    bool wasPressed() {
        int current = digitalRead(pin);
        bool pressed = (lastState == HIGH && current == LOW);
        lastState = current;
        return pressed;
    }

private:
    int pin;
    int lastState;
};

#endif
