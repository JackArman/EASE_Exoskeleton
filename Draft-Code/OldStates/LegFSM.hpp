#ifndef LEGFSM_HPP
#define LEGFSM_HPP

#include "GaitState.hpp"
#include "Stance.hpp"

class MotorController;

class LegFSM {
public:
    LegFSM(bool isLeft) : isLeft(isLeft), state(nullptr), sensorTriggered(false) {}

    void begin(MotorController* controller) {
        state = new Stance();
        state->enter(controller, this);
    }
    void update(MotorController* controller, unsigned long now) {




        // returns the upated new state from within the state - does the individual leg context objects know to transition state????
        GaitState* newState = state->update(controller, this, now);
        // will return same state if not needed to switch
        if (newState != state) {
            state->exit(controller, this);
            delete state;
            state = newState;
            state->enter(controller, this);
        }
    }
    
    void setSensorTriggered(bool triggered) { sensorTriggered = triggered; }
    bool isSensorTriggered() const { return sensorTriggered; }
    bool isLeftLeg() const { return isLeft; }



private:
    GaitState* state;
    bool isLeft;
    bool sensorTriggered;
};

#endif
