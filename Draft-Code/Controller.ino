class Controller {
private:
  StepState curLeftState;
  StepState curRightState;

public:
  Controller() {
    // optionally initialize StepState here
  }

  void checkTriggerTransition() {
    // Read sensor (force or IMU)
    // If foot off ground or on ground → update curLeftState / curRightState
    // Call transition() if needed
  }

  void transition() {
    // Handle switching left/right swing/stance
    // Possibly update timestamps or flags in StepState
  }

  void setMotorPositions(Motor& lHip, Motor& lKnee, Motor& rHip, Motor& rKnee) {
    // Read from curLeftState / curRightState
    // Set motor position, velocity, torque accordingly
    // lHip.setPos(curLeftState.getHipTargetPos());
    // rKnee.setTorque(curRightState.getKneeTorque());
    // ...
  }

  void loop(Motor& lHip, Motor& lKnee, Motor& rHip, Motor& rKnee) {
    checkTriggerTransition();
    setMotorPositions(lHip, lKnee, rHip, rKnee);
  }
};
