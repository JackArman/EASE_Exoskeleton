#ifndef MOTORCONTROLLER_HPP
#define MOTORCONTROLLER_HPP

#include <SPI.h>
#include <mcp2515.h>
#include "LegFSM.hpp"
#include "ForceSensor.hpp"
#include "Button.hpp"
#include <cstdint>
#include "GeneralState.hpp"
#include "InitialContactState.hpp"


struct can_frame canMsg;
MCP2515 mcp2515(5);


// Assumes all gait arrays are the same length for simplicity
int gaitLength = sizeof(hipLeftGait) / sizeof(hipLeftGait[0]);
int gaitIndex = 0;

// Unique motor IDs (replace with actual IDs)
const uint8_t MOTOR_ID_LEFT_HIP = 0x03;
const uint8_t MOTOR_ID_RIGHT_HIP = 0x01;
const uint8_t MOTOR_ID_LEFT_KNEE = 0x04;
const uint8_t MOTOR_ID_RIGHT_KNEE = 0x02;

GeneralState* currState = InitialContactState(this);


class MotorController {
public:
    void begin() {
        
      SPI.begin();
      mcp2515.reset();
      mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
      mcp2515.setNormalMode();

      delay(10000);
      // debug statement
      Serial.println("MotorController initialized");


      // NOTE: currently added two sensors for simple left/right checks - will likely require more
      leftSensor.begin();
      rightSensor.begin();
    }

    void update() {
        // if (button.wasPressed()) {
        //     // do something/manually override state change here depending on what we want
        // }

        if (currState->checkTransitionState()) {
          currState->transitionState();
        }

        LegData newData = currState->getLegData();
        // unsigned long now = millis();
        sendAllJointCommands(newData);
    }

    void changeState(GeneralState newState) {
      currState = newState;
    }

    // TODO: integrate force sensors
    // bool forceSensorLeftTriggered() {
    //    return leftSensor.baselineTriggered(); 
    // }
    // bool forceSensorRightTriggered() {
    //    return rightSensor.baselineTriggered(); 
    // }

private:
    // CAN comms
    MCP2515 mcp = MCP2515(5);
    struct can_frame canMsg;

    // Joint targets
    float leftHipPos = 0.0, leftKneePos = 0.0;
    float leftHipVel = 0.0, leftKneeVel = 0.0;
    float rightHipPos = 0.0, rightKneePos = 0.0;
    float rightHipVel = 0.0, rightKneeVel = 0.0;

    const uint8_t LEFT_HIP_ID   = 0x61;
    const uint8_t LEFT_KNEE_ID  = 0x62;
    const uint8_t RIGHT_HIP_ID  = 0x63;
    const uint8_t RIGHT_KNEE_ID = 0x68;

    const float KP = 40.0;
    const float KD = 2.0;
    const float TORQUE = 0.1;

    // Helper: Convert float to fixed-point unsigned int
    int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
        float span = x_max - x_min;
        float offset = x_min;
        x = constrain(x, x_min, x_max);
        return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
    }

    void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
      int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
      int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
      int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
      int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
      int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

      uint32_t can_id = (0x08 << 8) | motor_id;
      canMsg.can_id   = 0x80000000UL | can_id;  // Extended frame
      canMsg.can_dlc  = 8;

      canMsg.data[0] = kp_int >> 4;
      canMsg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
      canMsg.data[2] = kd_int & 0xFF;
      canMsg.data[3] = p_int >> 8;
      canMsg.data[4] = p_int & 0xFF;
      canMsg.data[5] = v_int >> 4;
      canMsg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
      canMsg.data[7] = t_int & 0xFF;

      mcp.sendMessage(&canMsg);
    }

    void sendAllJointCommands(LegData data) {
      sendMITCommand(data[2], leftHipVel, KP, KD, TORQUE, LEFT_HIP_ID);
      sendMITCommand(data[0], leftKneeVel, KP, KD, TORQUE, LEFT_KNEE_ID);
      sendMITCommand(data[3], rightHipVel, KP, KD, TORQUE, RIGHT_HIP_ID);
      sendMITCommand(data[1], rightKneeVel, KP, KD, TORQUE, RIGHT_KNEE_ID);
    }


    ForceSensor leftSensor = ForceSensor();
    ForceSensor rightSensor = ForceSensor();
    // select digital pin for button
    Button button = Button(2);
};

#endif
