#ifndef SWINGUP_HPP
#define SWINGUP_HPP

#include "GaitState.hpp"
#include "SwingDown.hpp"
#include "MotorController.hpp"
#include "LegFSM.hpp"

class SwingUp : public GaitState {
public:
    void enter(MotorController* m, LegFSM* fsm) override {
        startTime = millis();

        if (fsm->isLeftLeg()) {
            m->setLeftLegTargets(0.4, 0.6, 0.8, 1.2);  // pos, vel
        } else {
            m->setRightLegTargets(0.4, 0.6, 0.8, 1.2);
        }
    }

    GaitState* update(MotorController* m, LegFSM* fsm, unsigned long now) override {
        if (fsm->isSensorTriggered()) {
            return new SwingDown();
        }
        return this;
    }

    void exit(MotorController*, LegFSM*) override {}

private:
    unsigned long startTime;
};

#endif
