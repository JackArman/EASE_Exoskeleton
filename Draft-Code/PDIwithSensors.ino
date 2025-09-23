#include <SPI.h>
#include <mcp2515.h>

// CAN Frame Setup
struct can_frame canMsg;
MCP2515 mcp2515(5);

// Force Sensor Pin
const int sensorPin = 34;

// --- Helper: Float to Unsigned Int ---
int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  float offset = x_min;
  if (x < x_min) x = x_min;
  if (x > x_max) x = x_max;
  return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
}

// Motor IDs
const uint8_t LEFT_HIP_ID   = 0x61;
const uint8_t LEFT_KNEE_ID  = 0x62;
const uint8_t RIGHT_HIP_ID  = 0x63;
const uint8_t RIGHT_KNEE_ID = 0x68;

// Step Timing
unsigned long lastStepTime = 0;
unsigned long stepInterval = 1000;
bool leftLegForward = true;

// Gait Parameters
const float HIP_SWING     = 0.4;
const float KNEE_SWING_F  = 0.7;
const float KNEE_SWING_B  = -0.3;
const float KP = 40.0;
const float KD = 2.0;
const float TORQUE = 0.1;
const float VEL_HIP = 0.8;
const float VEL_KNEE = 1.2;

// Sensor Printing Timing
unsigned long lastSensorPrint = 0;
const unsigned long sensorInterval = 100;  // Every 100ms

// --- Send MIT Motor Command ---
void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
  int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
  int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
  int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
  int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
  int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

  uint32_t can_id = (0x08 << 8) | motor_id;
  canMsg.can_id   = 0x80000000UL | can_id;
  canMsg.can_dlc  = 8;

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

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 0–4095 ADC
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("Exoskeleton Control + Sensor Initialized");
}

void loop() {
  unsigned long now = millis();

  // --- Step Logic ---
  if (now - lastStepTime > stepInterval) {
    leftLegForward = !leftLegForward;
    lastStepTime = now;
    Serial.println(leftLegForward ? "Left leg forward" : "Right leg forward");
  }

  // --- Joint Targets ---
  float l_hip_pos  = leftLegForward ?  HIP_SWING  : -HIP_SWING;
  float l_knee_pos = leftLegForward ?  KNEE_SWING_F : KNEE_SWING_B;
  float l_hip_vel  = leftLegForward ?  VEL_HIP    : -VEL_HIP;
  float l_knee_vel = leftLegForward ?  VEL_KNEE   : -VEL_KNEE;

  float r_hip_pos  = leftLegForward ? -HIP_SWING  :  HIP_SWING;
  float r_knee_pos = leftLegForward ? KNEE_SWING_B : KNEE_SWING_F;
  float r_hip_vel  = leftLegForward ? -VEL_HIP    :  VEL_HIP;
  float r_knee_vel = leftLegForward ? -VEL_KNEE   :  VEL_KNEE;

  // --- Motor Commands ---
  sendMITCommand(l_hip_pos,  l_hip_vel,  KP, KD, TORQUE, LEFT_HIP_ID);
  sendMITCommand(l_knee_pos, l_knee_vel, KP, KD, TORQUE, LEFT_KNEE_ID);
  sendMITCommand(r_hip_pos,  r_hip_vel,  KP, KD, TORQUE, RIGHT_HIP_ID);
  sendMITCommand(r_knee_pos, r_knee_vel, KP, KD, TORQUE, RIGHT_KNEE_ID);

  // --- Sensor Read and Print (every 100ms) ---
  if (now - lastSensorPrint > sensorInterval) {
    lastSensorPrint = now;
    int analogValue = analogRead(sensorPin);
    float voltage = analogValue * (3.3 / 4095.0);

    Serial.print("[Sensor] Analog: ");
    Serial.print(analogValue);
    Serial.print("  Voltage: ");
    Serial.println(voltage, 3);
  }

  // --- Loop Timing: 10ms cycle ---
  delay(10);  // Keep 100Hz motor command loop
}
