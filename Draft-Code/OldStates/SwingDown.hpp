#ifndef SWINGDOWN_HPP
#define SWINGDOWN_HPP

#include "GaitState.hpp"
#include "Stance.hpp"
#include "MotorController.hpp"
#include "LegFSM.hpp"

class SwingDown : public GaitState {
public:
    void enter(MotorController* m, LegFSM* fsm) override {
        if (fsm->isLeftLeg()) {
            m->setLeftLegTargets(-0.1, -0.3, -0.5, -1.0);
        } else {
            m->setRightLegTargets(-0.1, -0.3, -0.5, -1.0);
        }
    }

    GaitState* update(MotorController* m, LegFSM* fsm, unsigned long now) override {
        if (fsm->isSensorTriggered()) {
            return new Stance();
        }
        return this;
    }

    void exit(MotorController*, LegFSM*) override {}
};

#endif
