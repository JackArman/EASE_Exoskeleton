// EASE Exoskeleton – MIT Position Control (Broadcast-friendly, with logging)
// Controls four lower-limb motors using prerecorded gait trajectories
// Logs CAN feedback with high-resolution ESP32 timestamps

#include <SPI.h>
#include <mcp2515.h>
#include <cstdint>
#include "esp_timer.h"          // ESP32 high-resolution timer

// CAN message structure
struct can_frame canMsg;
MCP2515 mcp2515(5);             // MCP2515 connected to SPI CS pin 5

const int dialPin = 34;         // Analog pin connected to potentiometer (speed dial)

// Potentiometer states
enum dialState {
  DIAL_OFF = -1,   // No motion
  DIAL_LOW = 40,   // Slow walking
  DIAL_MEDIUM = 20,// Normal walking
  DIAL_HIGH = 0    // Fast walking
};

// Track maximum amplitudes for logging (optional)
double maxAmp1 = 0, maxAmp2 = 0, maxAmp3 = 0, maxAmp4 = 0, total = 0, maxTotal = 0;

// Counter for logging gait steps
long gait_step_counter = 0;

// CAN feedback storage
uint8_t msgDataRightHip[8]  = {0};
uint8_t msgDataRightKnee[8] = {0};
uint8_t msgDataLeftHip[8]   = {0};
uint8_t msgDataLeftKnee[8]  = {0};

// Timer start for logging
int64_t t0_us = 0;              // Start timestamp in microseconds
float gaitTime = 0;

// Joint angles (for calculations)
double R_hip_ang = 0.0, R_knee_ang = 0.0, L_hip_ang = 0.0, L_knee_ang = 0.0;

// Number of samples per gait cycle
#define GAIT_LENGTH 100

// Pre-recorded gait trajectories for left and right legs
double L_knee[GAIT_LENGTH] = { /* 100 values for left knee trajectory */ };
double L_hip[GAIT_LENGTH]  = { /* 100 values for left hip trajectory */ };
double R_knee[GAIT_LENGTH] = { /* 100 values for right knee trajectory */ };
double R_hip[GAIT_LENGTH]  = { /* 100 values for right hip trajectory */ };

// Offset to adjust gait synchronization
int offset = 90;

// Last potentiometer reading (for smoothing)
int lastDialValue = 0;

// Unique motor IDs – update to match hardware
const uint8_t MOTOR_ID_LEFT_HIP  = 0x03;
const uint8_t MOTOR_ID_RIGHT_HIP = 0x01;
const uint8_t MOTOR_ID_LEFT_KNEE = 0x04;
const uint8_t MOTOR_ID_RIGHT_KNEE= 0x02;

// ---------- Helper Functions ----------

// Converts a float value to an unsigned integer for CAN message
static inline int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  if (x < x_min) x = x_min;
  if (x > x_max) x = x_max;
  return (int)((x - x_min) * ((float)((1u << bits) - 1u) / span));
}

// Drain CAN RX buffer for a short period to avoid overflow
// Stores the latest message by motor ID
static inline void drainRXUntil(uint32_t time_us) {
  const uint32_t t0 = micros();
  while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    uint8_t motor_id = (uint8_t)(canMsg.can_id & 0xFF); // Motor ID in low 8 bits
    if (motor_id == MOTOR_ID_RIGHT_HIP)       memcpy(msgDataRightHip,  canMsg.data, 8);
    else if (motor_id == MOTOR_ID_RIGHT_KNEE) memcpy(msgDataRightKnee, canMsg.data, 8);
    else if (motor_id == MOTOR_ID_LEFT_KNEE)  memcpy(msgDataLeftKnee,  canMsg.data, 8);
    else if (motor_id == MOTOR_ID_LEFT_HIP)   memcpy(msgDataLeftHip,   canMsg.data, 8);
    if ((micros() - t0) >= time_us) break;
  }
}

// Open all MCP2515 filters to accept any message
static inline void openAllFiltersAndRollover() {
  mcp2515.setFilterMask(MCP2515::MASK0, true, 0x00000000);
  mcp2515.setFilterMask(MCP2515::MASK1, true, 0x00000000);
  for (int i = 0; i <= 5; i++) mcp2515.setFilter((MCP2515::RXF0 + i), true, 0x00000000);
}

// Clamp float value within min and max
static inline float clampf(float x, float x_min, float x_max) {
  if (x < x_min) return x_min;
  if (x > x_max) return x_max;
  return x;
}

// Send current/brake command to motor
static inline void sendCurrentBrake(float i_brake_A, uint8_t motor_id) {
  float i_cmd = fabsf(i_brake_A);
  i_cmd = clampf(i_cmd, -60.0f, 60.0f);  // Clamp to datasheet limits

  int32_t i_scaled = (int32_t)(i_cmd * 1000.0f); // Scale to 0-60000

  uint8_t buf[4];
  buf[0] = (i_scaled >> 24) & 0xFF;
  buf[1] = (i_scaled >> 16) & 0xFF;
  buf[2] = (i_scaled >>  8) & 0xFF;
  buf[3] = (i_scaled)       & 0xFF;

  uint32_t can_id = motor_id | ((uint32_t)0x02 << 8); // Mode ID for current/brake

  canMsg.can_id  = can_id | CAN_EFF_FLAG;  
  canMsg.can_dlc = 4;

  memcpy(canMsg.data, buf, 4);
  mcp2515.sendMessage(&canMsg);
}

// Send MIT motor command (position, velocity, gains, feedforward torque)
static inline void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
  int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
  int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
  int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
  int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
  int t_int  = float_to_uint(t_ff, -54.0f, 54.0f, 12);

  uint32_t can_id = ((uint32_t)0x08 << 8) | motor_id; 
  canMsg.can_id  = 0x80000000UL | can_id; // Extended frame
  canMsg.can_dlc = 8;

  canMsg.data[0] = kp_int >> 4;
  canMsg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
  canMsg.data[2] = kd_int & 0xFF;
  canMsg.data[3] = p_int >> 8;
  canMsg.data[4] = p_int & 0xFF;
  canMsg.data[5] = v_int >> 4;
  canMsg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
  canMsg.data[7] = t_int & 0xFF;

  mcp2515.sendMessage(&canMsg);
}

// Setup ESP32 and MCP2515
void setup() {
  Serial.begin(921600);   // Fast serial for logging
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ); // 1 Mbps CAN
  openAllFiltersAndRollover();
  mcp2515.setNormalMode();

  t0_us = esp_timer_get_time(); // Start timestamp
  delay(10000);                  // Optional delay to stabilize
  Serial.println("Multi-joint gait tracking started");
}

// Read dial and determine walking speed
dialState getDialState(int dialValue) {
  if (dialValue == 4095 && lastDialValue <= 4095 - 100) dialValue = lastDialValue;
  dialValue &= 0b1111111111100000;  // Smooth ADC noise
  lastDialValue = dialValue;

  int dialPercent = map(dialValue, 0, 4095, 0, 1000);
  if (dialPercent < 250) return DIAL_HIGH;
  if (dialPercent < 500) return DIAL_MEDIUM;
  if (dialPercent < 750) return DIAL_LOW;
  return DIAL_OFF;
}

// Decode feedback from motors
void decodeMotorFeedback(struct can_frame *msg) {
  if (msg->can_dlc < 8) return;
  uint16_t motor_id = (uint16_t)(msg->can_id & 0xFF); // Extract motor ID
  if (motor_id == MOTOR_ID_RIGHT_HIP)       memcpy(msgDataRightHip,  msg->data, 8);
  else if (motor_id == MOTOR_ID_RIGHT_KNEE) memcpy(msgDataRightKnee, msg->data, 8);
  else if (motor_id == MOTOR_ID_LEFT_KNEE)  memcpy(msgDataLeftKnee,  msg->data, 8);
  else if (motor_id == MOTOR_ID_LEFT_HIP)   memcpy(msgDataLeftHip,   msg->data, 8);
}

// Log gait data to Serial
void logGaitData(int LgaitIndex, int RgaitIndex) {
  if (gait_step_counter == 0) {
    Serial.println("TimeStep,Elapsed_us,L_Gait_Index,R_Gait_Index,RH[8],RK[8],LK[8],LH[8]");
  }

  if ((gait_step_counter % 5) != 0) { gait_step_counter++; return; } // Decimate

  int64_t elapsed_us = esp_timer_get_time() - t0_us;

  Serial.print(gait_step_counter); Serial.print(",");
  Serial.print((long long)elapsed_us); Serial.print(",");
  Serial.print(LgaitIndex); Serial.print(",");
  Serial.print(RgaitIndex); Serial.print(",");

  auto printRawData = [](uint8_t data[]) {
    for (int i = 0; i < 8; i++) {
      Serial.print(data[i]);
      if (i < 7) Serial.print(",");
    }
  };

  printRawData(msgDataRightHip);  Serial.print(",");
  printRawData(msgDataRightKnee); Serial.print(",");
  printRawData(msgDataLeftKnee);  Serial.print(",");
  printRawData(msgDataLeftHip);   Serial.println("");

  gait_step_counter++;
}

// Main control loop
void loop() {
  // Control parameters
  const float v_des = 0.0f;
  float kp = 40.0f;
  float kd = 2.0f;
  const float torque_ff_max_hip = 8.71875f;
  const float torque_ff_max_knee = 4.98375f; 

  int LgaitIndex = 0;
  int RgaitIndex = GAIT_LENGTH / 2;

  int dialValue = analogRead(dialPin);
  dialState dial = getDialState(dialValue);

  while (dial == DIAL_OFF) { // Wait until user turns dial on
    dialValue = analogRead(dialPin);
    dial = getDialState(dialValue);
    delay(20);
  }

  // Move legs smoothly to start position
  float iterations = 500.0f;
  for (int i = 1; i < iterations; i++) {
    int leftKnee  = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;

    sendMITCommand(-(R_hip[LgaitIndex] * 1.3f) * (i/iterations), v_des, kp-10, kd, -torque_ff_max_hip*(i/iterations), MOTOR_ID_LEFT_HIP);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand(-(R_knee[leftKnee] * 0.9f)*(i/iterations), v_des, kp-10, kd, -torque_ff_max_knee*(i/iterations), MOTOR_ID_LEFT_KNEE);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand( (R_hip[RgaitIndex] * 1.3f)*(i/iterations), v_des, kp-10, kd, torque_ff_max_hip*(i/iterations), MOTOR_ID_RIGHT_HIP);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand( (R_knee[rightKnee] * 0.9f)*(i/iterations), v_des, kp-10, kd, torque_ff_max_knee*(i/iterations), MOTOR_ID_RIGHT_KNEE);
    delayMicroseconds(200); drainRXUntil(500);
    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) decodeMotorFeedback(&canMsg);
    delay(1);
  }

  // Main gait loop
  while (dial != DIAL_OFF) {
    int leftKnee  = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;

    dialValue = analogRead(dialPin);
    dial = getDialState(dialValue);
    if (dial == DIAL_OFF) break;

    sendMITCommand(-(R_hip[LgaitIndex])*1.3, v_des, kp, kd, -torque_ff_max_hip, MOTOR_ID_LEFT_HIP);
    sendMITCommand(-(R_knee[leftKnee]*0.7)*1.3, v_des, kp, kd, -torque_ff_max_knee, MOTOR_ID_LEFT_KNEE);
    sendMITCommand((R_hip[RgaitIndex])*1.3, v_des, kp, kd, torque_ff_max_hip, MOTOR_ID_RIGHT_HIP);
    sendMITCommand((R_knee[rightKnee]*0.7)*1.3, v_des, kp, kd, torque_ff_max_knee, MOTOR_ID_RIGHT_KNEE);

    LgaitIndex = (LgaitIndex + 1) % GAIT_LENGTH;
    RgaitIndex = (RgaitIndex + 1) % GAIT_LENGTH;

    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) decodeMotorFeedback(&canMsg);

    delay(20 + dial); // Adjust speed based on dial
  }

  // Smoothly return legs to zero position after dial off
  iterations = 300.0;
  kp = 150; kd = kp/3;
  for (int i = 1; i < iterations; i++) {
    int leftKnee  = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;

    double R_hip_ang   = -(R_hip[LgaitIndex]*1.3f) * (1 - i/iterations);
    double R_knee_ang  = -(R_knee[leftKnee]*0.9f) * (1 - i/iterations);
    double L_hip_ang   =  (R_hip[RgaitIndex]*1.3f) * (1 - i/iterations);
    double L_knee_ang  =  (R_knee[rightKnee]*0.9f) * (1 - i/iterations);

    sendMITCommand(R_hip_ang, v_des, kp, kd, -torque_ff_max_hip*(1 - i/iterations), MOTOR_ID_LEFT_HIP);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand(R_knee_ang, v_des, kp, kd, -torque_ff_max_knee*(1 - i/iterations), MOTOR_ID_LEFT_KNEE);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand(L_hip_ang, v_des, kp, kd, torque_ff_max_hip*(1 - i/iterations), MOTOR_ID_RIGHT_HIP);
    delayMicroseconds(150); drainRXUntil(400);
    sendMITCommand(L_knee_ang, v_des, kp, kd, torque_ff_max_knee*(1 - i/iterations), MOTOR_ID_RIGHT_KNEE);
    delayMicroseconds(200); drainRXUntil(500);

    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) decodeMotorFeedback(&canMsg);
    delay(1);
  }

  Serial.println("Finished control loop, restarting");
}
