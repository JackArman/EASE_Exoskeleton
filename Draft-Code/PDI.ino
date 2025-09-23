#include <SPI.h>          // For SPI communication with MCP2515
#include <mcp2515.h>      // MCP2515 CAN controller library

// --- CAN Frame Setup ---
struct can_frame canMsg;         // CAN frame structure
MCP2515 mcp2515(5);              // MCP2515 object, CS pin on D5

// --- Helper: Convert float to packed unsigned int for CAN ---
int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  float offset = x_min;

  // Clamp value within limits
  if (x < x_min) x = x_min;
  if (x > x_max) x = x_max;

  // Map float to unsigned int range
  return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
}

// --- Motor IDs (per joint) ---
const uint8_t LEFT_HIP_ID   = 0x61;
const uint8_t LEFT_KNEE_ID  = 0x62;
const uint8_t RIGHT_HIP_ID  = 0x63;
const uint8_t RIGHT_KNEE_ID = 0x68;

// --- Step timing ---
unsigned long lastStepTime = 0;     // Last time we switched legs
unsigned long stepInterval = 1000;  // Step every 1 second
bool leftLegForward = true;         // Toggle direction of step

// --- Position targets for walking (in radians) ---
const float HIP_SWING     = 0.4;    // Forward/backward hip range
const float KNEE_SWING_F  = 0.7;    // Knee flexion (forward)
const float KNEE_SWING_B  = -0.3;   // Knee extension (backward)

// --- Velocity and torque settings ---
const float KP = 40.0;              // Proportional gain
const float KD = 2.0;               // Derivative gain
const float TORQUE = 0.1;           // Feedforward torque (Nm)
const float VEL_HIP = 0.8;          // Hip joint velocity
const float VEL_KNEE = 1.2;         // Knee joint velocity

// --- Send command using MIT protocol over CAN ---
void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
  // 1. Convert floats to appropriate fixed-point format
  int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
  int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
  int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
  int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
  int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

  // 2. Construct extended CAN ID (MIT format)
  uint32_t can_id = (0x08 << 8) | motor_id;        // E.g., 0x0861
  canMsg.can_id   = 0x80000000UL | can_id;         // Set extended frame flag
  canMsg.can_dlc  = 8;                             // 8-byte data length

  // 3. Pack data into 8-byte CAN frame
  canMsg.data[0] = kp_int >> 4;
  canMsg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
  canMsg.data[2] = kd_int & 0xFF;
  canMsg.data[3] = p_int >> 8;
  canMsg.data[4] = p_int & 0xFF;
  canMsg.data[5] = v_int >> 4;
  canMsg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
  canMsg.data[7] = t_int & 0xFF;

  // 4. Send over CAN bus
  mcp2515.sendMessage(&canMsg);
}

void setup() {
  Serial.begin(115200);    // For debugging
  SPI.begin();             // Start SPI interface

  // MCP2515 Setup
  mcp2515.reset();                                 // Reset CAN controller
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);      // 1 Mbps CAN speed, 8 MHz crystal
  mcp2515.setNormalMode();                         // Enter normal (send/receive) mode

  Serial.println("Exoskeleton Walking Control Initialized");
}

void loop() {
  unsigned long now = millis();  // Current time

  // --- STEP LOGIC ---
  // Switch leg direction every `stepInterval`
  if (now - lastStepTime > stepInterval) {
    leftLegForward = !leftLegForward;   // Toggle direction
    lastStepTime = now;

    Serial.println(leftLegForward ? "Left leg forward" : "Right leg forward");
  }

  // --- ASSIGN VALUES PER JOINT BASED ON DIRECTION ---

  // LEFT LEG
  float l_hip_pos  = leftLegForward ?  HIP_SWING  : -HIP_SWING;
  float l_knee_pos = leftLegForward ?  KNEE_SWING_F : KNEE_SWING_B;
  float l_hip_vel  = leftLegForward ?  VEL_HIP    : -VEL_HIP;
  float l_knee_vel = leftLegForward ?  VEL_KNEE   : -VEL_KNEE;

  // RIGHT LEG (opposite phase)
  float r_hip_pos  = leftLegForward ? -HIP_SWING  :  HIP_SWING;
  float r_knee_pos = leftLegForward ? KNEE_SWING_B : KNEE_SWING_F;
  float r_hip_vel  = leftLegForward ? -VEL_HIP    :  VEL_HIP;
  float r_knee_vel = leftLegForward ? -VEL_KNEE   :  VEL_KNEE;

  // --- SEND MIT COMMANDS TO EACH MOTOR ---
  // Left leg
  sendMITCommand(l_hip_pos,  l_hip_vel,  KP, KD, TORQUE, LEFT_HIP_ID);
  sendMITCommand(l_knee_pos, l_knee_vel, KP, KD, TORQUE, LEFT_KNEE_ID);

  // Right leg
  sendMITCommand(r_hip_pos,  r_hip_vel,  KP, KD, TORQUE, RIGHT_HIP_ID);
  sendMITCommand(r_knee_pos, r_knee_vel, KP, KD, TORQUE, RIGHT_KNEE_ID);

  // --- LOOP RATE CONTROL ---
  delay(10);  // Run loop at 100Hz
}
