#ifndef STANCE_HPP
#define STANCE_HPP

#include "GaitState.hpp"
#include "SwingUp.hpp"
#include "MotorController.hpp"
#include "LegFSM.hpp"

class Stance : public GaitState {
public:
    void enter(MotorController* m, LegFSM* fsm) override {
        if (fsm->isLeftLeg()) {
            m->setLeftLegTargets(0.0, 0.0, 0.0, 0.0);  // Hold position
        } else {
            m->setRightLegTargets(0.0, 0.0, 0.0, 0.0);
        }
        lastTransitionTime = millis();
    }

    GaitState* update(MotorController* m, LegFSM* fsm, unsigned long now) override {
        if (now - lastTransitionTime > 1000) {  // 1 second in stance
            return new SwingUp();
        }
        return this;
    }

    void exit(MotorController*, LegFSM*) override {}

private:
    unsigned long lastTransitionTime;
};

#endif
