#ifndef SWINGLEFT_HPP
#define SWINGLEFT_HPP

#include "GaitState.hpp"
#include "MotorController.hpp"

class SwingLeft : public GaitState {
public:
    void enter(MotorController* motor) override {
        motor->setLegPositions(+0.4, +0.7, -0.4, -0.3);  // l_hip, l_knee, r_hip, r_knee
        motor->setLegVelocities(+0.8, +1.2, -0.8, -1.2);
        startTime = millis();
    }

    GaitState* update(MotorController* motor, unsigned long now) override {
        if (now - startTime > motor->stepDuration()) {
            return new SwingRight();
        }
        return this;
    }

    void exit(MotorController* motor) override {}

private:
    unsigned long startTime;
};

#endif
