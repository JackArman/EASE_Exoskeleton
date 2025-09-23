
#include "LegData.hpp"
#include "MotorController.hpp"

class GeneralState {
public:
    GeneralState(MotorController* mc) {
        this.mc = mc;
    }
    virtual bool checkTransitionState() = 0;
    virtual LegData getLegData() = 0;
    virtual void transitionState() = 0;

private:
    MotorController* mc;
};

