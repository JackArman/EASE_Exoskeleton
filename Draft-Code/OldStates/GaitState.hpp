#ifndef GAITSTATE_HPP
#define GAITSTATE_HPP

class MotorController;
class LegFSM;

class GaitState {
public:
    virtual ~GaitState() {}
    virtual void enter(MotorController* controller, LegFSM* fsm) = 0;
    virtual GaitState* update(MotorController* controller, LegFSM* fsm, unsigned long now) = 0;
    virtual void exit(MotorController* controller, LegFSM* fsm) = 0;
};

#endif
