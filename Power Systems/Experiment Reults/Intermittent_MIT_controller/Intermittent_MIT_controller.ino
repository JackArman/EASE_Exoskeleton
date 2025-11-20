// revised code (broadcast-friendly, fair RX handling) + Elapsed_us logging

#include <SPI.h>
#include <mcp2515.h>
#include <cstdint>
#include "esp_timer.h"          // <<< added: ESP32 high-resolution timer

struct can_frame canMsg;
MCP2515 mcp2515(5);

const int dialPin = 34;  // ADC pin

enum dialState {
  DIAL_OFF = -1,
  DIAL_LOW = 40,
  DIAL_MEDIUM = 20,
  DIAL_HIGH = 0
};

double maxAmp1 = 0;
double maxAmp2 = 0;
double maxAmp3 = 0;
double maxAmp4 = 0;
double total = 0;
double maxTotal = 0;
long gait_step_counter = 0;

uint8_t msgDataRightHip[8]  = {0};
uint8_t msgDataRightKnee[8] = {0};
uint8_t msgDataLeftHip[8]   = {0};
uint8_t msgDataLeftKnee[8]  = {0};

int64_t t0_us = 0;              // <<< added: logging start time (microseconds)
float gaitTime = 0;
double R_hip_ang = 0.0;
double R_knee_ang = 0.0;
double L_hip_ang = 0.0;
double L_knee_ang = 0.0;

#define GAIT_LENGTH 100

double L_knee[GAIT_LENGTH] = {
0.042200, 0.059562, 0.092671, 0.141522, 0.204591, 0.278567, 0.358855, 0.440760, 0.520656, 0.596433,
0.667199, 0.732766, 0.793217, 0.848651, 0.899057, 0.944284, 0.984064, 1.018047, 1.045814, 1.066864,
1.080587, 1.086280, 1.083212, 1.070786, 1.048739, 1.017316, 0.977325, 0.930024, 0.876881, 0.819280,
0.758340, 0.694954, 0.630043, 0.564828, 0.500893, 0.439959, 0.383527, 0.332620, 0.287736, 0.248939,
0.215986, 0.188439, 0.165738, 0.147250, 0.132296, 0.120221, 0.110498, 0.102846, 0.097284, 0.094056,
0.093459, 0.095640, 0.100468, 0.107502, 0.116065, 0.125387, 0.134770, 0.143711, 0.151928, 0.159294,
0.165752, 0.171276, 0.175892, 0.179761, 0.183235, 0.186847, 0.191192, 0.196767, 0.203835, 0.212359,
0.222038, 0.232383, 0.242834, 0.252879, 0.262185, 0.270692, 0.278621, 0.286371, 0.294319, 0.302627,
0.311129, 0.319331, 0.326490, 0.331732, 0.334175, 0.333073, 0.327958, 0.318736, 0.305691, 0.289373,
0.270381, 0.249164, 0.225926, 0.200732, 0.173782, 0.145765, 0.118156, 0.093322, 0.074331, 0.064467
};

double L_hip[GAIT_LENGTH] = {
0.292550, 0.297928, 0.304605, 0.314464, 0.326739, 0.340090, 0.352850, 0.363396, 0.370504, 0.373525,
0.372359, 0.367321, 0.358969, 0.347929, 0.334727, 0.319662, 0.302772, 0.283915, 0.262917, 0.239725,
0.214489, 0.187544, 0.159319, 0.130239, 0.100649, 0.070755, 0.040595, 0.010050, -0.021057, -0.052805,
-0.085016, -0.117162, -0.148385, -0.177646, -0.203951, -0.226550, -0.245034, -0.259313, -0.269508, -0.275855,
-0.278642, -0.278197, -0.274901, -0.269205, -0.261621, -0.252683, -0.242877, -0.232542, -0.221803, -0.210577,
-0.198666, -0.185914, -0.172330, -0.158155, -0.143842, -0.129972, -0.117101, -0.105598, -0.095527, -0.086628,
-0.078398, -0.070250, -0.061665, -0.052281, -0.041909, -0.030488, -0.018052, -0.004705, 0.009396, 0.024051,
0.039038, 0.054131, 0.069120, 0.083820, 0.098116, 0.112010, 0.125649, 0.139305, 0.153324, 0.168035,
0.183673, 0.200304, 0.217761, 0.235617, 0.253211, 0.269742, 0.284423, 0.296648, 0.306110, 0.312815,
0.317008, 0.319036, 0.319227, 0.317822, 0.314967, 0.310784, 0.305515, 0.299675, 0.294106, 0.289862
};

double R_knee[GAIT_LENGTH] = {
0.037000, 0.052771, 0.084629, 0.130832, 0.188328, 0.253307, 0.322092, 0.391888, 0.461043, 0.528814,
0.594912, 0.659118, 0.721086, 0.780312, 0.836206, 0.888165, 0.935595, 0.977879, 1.014357, 1.044349,
1.067232, 1.082515, 1.089876, 1.089130, 1.080177, 1.062958, 1.037468, 1.003836, 0.962450, 0.914084,
0.859921, 0.801478, 0.740430, 0.678434, 0.616989, 0.557384, 0.500702, 0.447811, 0.399288, 0.355357,
0.315940, 0.280827, 0.249841, 0.222902, 0.199944, 0.180784, 0.165032, 0.152124, 0.141447, 0.132526,
0.125131, 0.119250, 0.114934, 0.112159, 0.110768, 0.110518, 0.111165, 0.112556, 0.114696, 0.117765,
0.122043, 0.127776, 0.135032, 0.143645, 0.153282, 0.163575, 0.174242, 0.185119, 0.196120, 0.207169,
0.218164, 0.228979, 0.239484, 0.249582, 0.259220, 0.268387, 0.277085, 0.285280, 0.292881, 0.299740,
0.305674, 0.310455, 0.313749, 0.315066, 0.313789, 0.309336, 0.301404, 0.290163, 0.276288, 0.260765,
0.244510, 0.227946, 0.210720, 0.191788, 0.169891, 0.144314, 0.115610, 0.085967, 0.059021, 0.039173
};

double R_hip[GAIT_LENGTH] = {
0.330000, 0.331301, 0.335672, 0.343296, 0.353111, 0.363612, 0.373177, 0.380432, 0.384554, 0.385413,
0.383465, 0.379469, 0.374125, 0.367799, 0.360427, 0.351604, 0.340792, 0.327532, 0.311585, 0.292979,
0.271981, 0.249010, 0.224518, 0.198860, 0.172211, 0.144572, 0.115859, 0.086032, 0.055214, 0.023762,
-0.007752, -0.038644, -0.068275, -0.096169, -0.122032, -0.145656, -0.166822, -0.185285, -0.200851, -0.213446,
-0.223136, -0.230068, -0.234403, -0.236298, -0.235952, -0.233664, -0.229848, -0.224949, -0.219312, -0.213080,
-0.206199, -0.198526, -0.189985, -0.180683, -0.170927, -0.161121, -0.151609, -0.142530, -0.133761, -0.124958,
-0.115683, -0.105567, -0.094431, -0.082317, -0.069412, -0.055917, -0.041935, -0.027455, -0.012408, 0.003225,
0.019357, 0.035805, 0.052363, 0.068890, 0.085359, 0.101854, 0.118501, 0.135401, 0.152584, 0.170029,
0.187704, 0.205571, 0.223518, 0.241274, 0.258375, 0.274250, 0.288403, 0.300598, 0.310954, 0.319869,
0.327803, 0.335008, 0.341347, 0.346295, 0.349147, 0.349355, 0.346828, 0.342093, 0.336243, 0.330710
};

int offset = 90;

// Unique motor IDs (replace with actual IDs)
const uint8_t MOTOR_ID_LEFT_HIP  = 0x03;
const uint8_t MOTOR_ID_RIGHT_HIP = 0x01;
const uint8_t MOTOR_ID_LEFT_KNEE = 0x04;
const uint8_t MOTOR_ID_RIGHT_KNEE= 0x02;

// ---------- helpers ----------
static inline int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  if (x < x_min) x = x_min;
  if (x > x_max) x = x_max;
  return (int)((x - x_min) * ((float)((1u << bits) - 1u) / span));
}

// drain RX for up to time_us microseconds; store latest by motor_id (broadcast-friendly)
static inline void drainRXUntil(uint32_t time_us) {
  const uint32_t t0 = micros();
  while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    uint8_t motor_id = (uint8_t)(canMsg.can_id & 0xFF); // Drive ID in low 8 bits
    if (motor_id == MOTOR_ID_RIGHT_HIP)       memcpy(msgDataRightHip,  canMsg.data, 8);
    else if (motor_id == MOTOR_ID_RIGHT_KNEE) memcpy(msgDataRightKnee, canMsg.data, 8);
    else if (motor_id == MOTOR_ID_LEFT_KNEE)  memcpy(msgDataLeftKnee,  canMsg.data, 8);
    else if (motor_id == MOTOR_ID_LEFT_HIP)   memcpy(msgDataLeftHip,   canMsg.data, 8);
    if ((micros() - t0) >= time_us) break;
  }
}

// open accept-all filters and enable RXB0 rollover (BUKT) into RXB1
static inline void openAllFiltersAndRollover() {
  mcp2515.setFilterMask(MCP2515::MASK0, true, 0x00000000);
  mcp2515.setFilterMask(MCP2515::MASK1, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF0, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF1, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF2, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF3, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF4, true, 0x00000000);
  mcp2515.setFilter(MCP2515::RXF5, true, 0x00000000);
  // optional: enable rollover if your fork exposes it
  // mcp2515.setRollover(true);
}

// ===== Current Brake / Current Mode ranges (SET THESE FROM DATASHEET) =====
const float I_BRAKE_MIN = -60.0f;   // <<< SET FROM DATASHEET (min phase current, A)
const float I_BRAKE_MAX =  60.0f;   // <<< SET FROM DATASHEET (max phase current, A)

// If the datasheet uses a different mode ID for current brake, set it here.
// Your MIT mode uses 0x08 << 8 | motor_id; often current mode is 0x0C, 0x0B, etc.
const uint8_t CURRENT_BRAKE_MODE_ID = 0x02;   // <<< SET FROM DATASHEET (command ID for current / brake)

// Simple helper (same style as before)
static inline float clampf(float x, float x_min, float x_max) {
  if (x < x_min) return x_min;
  if (x > x_max) return x_max;
  return x;
}

static inline void sendCurrentBrake(float i_brake_A, uint8_t motor_id)
{
  // 1) Use magnitude, clamp to [0, 60] A
  float i_cmd = fabsf(i_brake_A);
  i_cmd = clampf(i_cmd, I_BRAKE_MIN, I_BRAKE_MAX);

  // 2) Scale to int32: current [A] * 1000 -> 0..60000
  int32_t i_scaled = (int32_t)(i_cmd * 1000.0f);

  // 3) Pack as big-endian (same as buffer_append_int32 in datasheet)
  uint8_t buf[4];
  buf[0] = (uint8_t)((i_scaled >> 24) & 0xFF);
  buf[1] = (uint8_t)((i_scaled >> 16) & 0xFF);
  buf[2] = (uint8_t)((i_scaled >>  8) & 0xFF);
  buf[3] = (uint8_t)( i_scaled        & 0xFF);

  // 4) 29-bit CAN ID: controller_id | (CAN_PACKET_SET_CURRENT_BRAKE << 8)
  uint32_t can_id = (uint32_t)motor_id |
                    ((uint32_t)0x02 << 8);

  canMsg.can_id  = can_id | CAN_EFF_FLAG;  // extended frame
  canMsg.can_dlc = 4;                      // only 4 data bytes

  canMsg.data[0] = buf[0];
  canMsg.data[1] = buf[1];
  canMsg.data[2] = buf[2];
  canMsg.data[3] = buf[3];

  // Remaining bytes are ignored because DLC=4

  mcp2515.sendMessage(&canMsg);
}

static inline void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
  int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
  int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
  int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
  int kd_int = float_to_uint(kd, 0.0f, 5.0f,   12);
  int t_int  = float_to_uint(t_ff, -54.0f, 54.0f, 12); //max is +-64Nm, but clamp to lower than max

  uint32_t can_id = ((uint32_t)0x08 << 8) | motor_id; // [28:8]=0x08, [7:0]=Drive ID
  canMsg.can_id  = 0x80000000UL | can_id;   // Extended frame flag
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

void setup() {
  Serial.begin(921600);   // faster serial to minimize blocking
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);  // 1 Mbps @ 8 MHz crystal (your hardware)
  openAllFiltersAndRollover();
  mcp2515.setNormalMode();

  t0_us = esp_timer_get_time();   // <<< added: capture start timestamp
  delay(10000);
  Serial.println("Multi-joint gait tracking started");
}

dialState getDialState(int dialValue) {
  int dialPercent = map(dialValue, 0, 4095, 0, 100);
  if (dialPercent < 25)  return DIAL_OFF;
  if (dialPercent < 50)  return DIAL_LOW;
  if (dialPercent < 75)  return DIAL_MEDIUM;
  return DIAL_HIGH;
}

void loop() {
  // Common control params
  const float v_des = 0.0f;
  float kp = 40.0f;
  float kd = 2.0f;
  const float torque_ff_max_hip = 8.71875f; //LOW SETTINGS
  const float torque_ff_max_knee = 4.98375f; 

  // const float torque_ff_max_hip = 26.15625; //HIGH SETTINGS
  // const float torque_ff_max_knee = 14.95125f;
  
  int LgaitIndex = 0;
  int RgaitIndex = GAIT_LENGTH / 2;
  

  // Scale LgaitIndex (assume GAIT_LENGTH >= 100)
  float rampFactor = (float)LgaitIndex / 100.0;  

  // Clamp rampFactor between 0 and 1
  if (rampFactor > 1.0) rampFactor = 1.0;
  if (rampFactor < 0.0) rampFactor = 0.0;

  // Compute ramped torque_ff
  float torque_ff_L_HIP = torque_ff_max_hip * rampFactor;
  float torque_ff_L_KNEE = torque_ff_max_knee * rampFactor;
  
  // Clamp rampFactor between 0 and 1
  if (rampFactor > 1.0) rampFactor = 1.0;
  if (rampFactor < 0.0) rampFactor = 0.0;


  // Scale RgaitIndex (assume GAIT_LENGTH >= 100)
  rampFactor = (float)RgaitIndex / 100.0;

  // Compute ramped torque_ff
  float torque_ff_R_HIP = torque_ff_max_hip * rampFactor;
  float torque_ff_R_KNEE = torque_ff_max_knee * rampFactor;

  delay(10);

  int dialValue = analogRead(dialPin);
  dialState dial = getDialState(dialValue);

  dial = DIAL_MEDIUM; // FOR NO DIAL

  while (dial == DIAL_OFF) {
    dialValue = analogRead(dialPin);
    dial = getDialState(dialValue);
  }

  Serial.println("Moving legs to start");

  // Move leg into position (with micro-gaps + RX drains to avoid burst collisions)
  float iterations = 500.0f;
  for (int i = 1; i < iterations; i++) {
    int leftKnee  = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;

    sendMITCommand(-(R_hip[LgaitIndex] * 1.3f) * (i/iterations),  v_des, kp  - 10, kd, -torque_ff_L_HIP * (i/iterations), MOTOR_ID_LEFT_HIP);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand(-(R_knee[leftKnee] * 0.9f)   * (i/iterations),  v_des, kp - 10, kd, -torque_ff_L_KNEE * (i/iterations), MOTOR_ID_LEFT_KNEE);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand( (R_hip[RgaitIndex] * 1.3f) * (i/iterations),  v_des, kp - 10, kd, torque_ff_R_HIP * (i/iterations), MOTOR_ID_RIGHT_HIP);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand( (R_knee[rightKnee] * 0.9f)  * (i/iterations),  v_des, kp - 10 , kd, torque_ff_R_KNEE * (i/iterations), MOTOR_ID_RIGHT_KNEE);
    delayMicroseconds(200); drainRXUntil(500);

    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      decodeMotorFeedback(&canMsg);
    }
    
    logGaitData(LgaitIndex, RgaitIndex);

    delay(1);
  }

  int exitCond = false;
  while (dial != DIAL_OFF) {
    int leftKnee = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;

    dialValue = analogRead(dialPin);
    dial = getDialState(dialValue);

    dial = DIAL_MEDIUM; // FOR NO PETENTIOMETER

    // if (dial == DIAL_OFF) {
    //   break;
    // }

    // Scale LgaitIndex (assume GAIT_LENGTH >= 100)
    rampFactor = (float)LgaitIndex / 100.0;  

    // Clamp rampFactor between 0 and 1
    if (rampFactor > 0.25) rampFactor = 0.25;
    if (rampFactor < 0.0) rampFactor = 0.0;

    // Compute ramped torque_ff
    torque_ff_L_HIP = torque_ff_max_hip * rampFactor;
    torque_ff_L_KNEE = torque_ff_max_knee * rampFactor;
    
    
    // Scale RgaitIndex (assume GAIT_LENGTH >= 100)
    rampFactor = (float)RgaitIndex / 100.0;

    // Clamp rampFactor between 0 and 1
    if (rampFactor > 0.25) rampFactor = 0.25;
    if (rampFactor < 0.0) rampFactor = 0.0;

    // Compute ramped torque_ff
    torque_ff_R_HIP = torque_ff_max_hip * rampFactor;
    torque_ff_R_KNEE = torque_ff_max_knee * rampFactor;

    sendMITCommand(-(R_hip[LgaitIndex]) * 1.3, v_des, kp, kd, -torque_ff_L_HIP, MOTOR_ID_LEFT_HIP);
    sendMITCommand(-(R_knee[leftKnee] * .7) * 1.3, v_des, kp, kd, -torque_ff_L_KNEE, MOTOR_ID_LEFT_KNEE);
    sendMITCommand((R_hip[RgaitIndex]) * 1.3, v_des, kp, kd, torque_ff_R_HIP, MOTOR_ID_RIGHT_HIP);
    sendMITCommand((R_knee[rightKnee] * .7) * 1.3, v_des, kp, kd, torque_ff_R_KNEE, MOTOR_ID_RIGHT_KNEE);
    LgaitIndex = (LgaitIndex + 1) % GAIT_LENGTH;
    RgaitIndex = (RgaitIndex + 1) % GAIT_LENGTH;
    if (LgaitIndex == 0) { 
      maxAmp1 = 0;
      maxAmp2 = 0;
    }

    int _delay = 20 + dial;
    // if (LgaitIndex == 0) {
    //   dial = DIAL_OFF; // FOR NO POTENTIOMETER
    // }
    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      decodeMotorFeedback(&canMsg);
    }

    logGaitData(LgaitIndex, RgaitIndex);

    //break when right leg reaches 0 (straight legged) and after 2 seconds has passed
    if (RgaitIndex  == 0) {
      if (exitCond) {
        break;
      }
      exitCond = true;
    }

    delay(_delay);
  }

    // MOVE LEGS BACK TO ZERO
  Serial.println("Moving legs to zero");
  iterations = 300.0;
  kp = 150;
  kd = kp/3;
  for (int i = 1; i < iterations; i++) {
    int leftKnee  = (LgaitIndex + offset) % GAIT_LENGTH;
    int rightKnee = (RgaitIndex + offset) % GAIT_LENGTH;
  
    double R_hip_ang = -(R_hip[LgaitIndex] * 1.3f) * (1 - i/iterations);
    double R_knee_ang = -(R_knee[leftKnee] * 0.9f) * (1 - i/iterations);
    double L_hip_ang = (R_hip[RgaitIndex] * 1.3f) * (1 - i/iterations);
    double L_knee_ang = (R_knee[rightKnee] * 0.9f) * (1 - i/iterations);

    sendMITCommand(R_hip_ang,  v_des, kp, kd, -torque_ff_L_HIP * (1 - i/iterations), MOTOR_ID_LEFT_HIP);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand(R_knee_ang,  v_des, kp, kd, -torque_ff_L_KNEE * (1 - i/iterations), MOTOR_ID_LEFT_KNEE);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand(L_hip_ang,  v_des, kp, kd, torque_ff_R_HIP * (1 - i/iterations), MOTOR_ID_RIGHT_HIP);
    delayMicroseconds(150); drainRXUntil(400);

    sendMITCommand(L_knee_ang,  v_des, kp, kd, torque_ff_R_KNEE * (1 - i/iterations), MOTOR_ID_RIGHT_KNEE);
    delayMicroseconds(200); drainRXUntil(500);

    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      decodeMotorFeedback(&canMsg);
    }
    
    logGaitData(LgaitIndex, RgaitIndex);
    delay(1);
  }
  Serial.println("finished control loop, restarting");

  //hold position while controlling position

  int hold_time = 3000;
  uint32_t t_start = millis();
  while (millis() - t_start < hold_time) {
    // stiff hold at zero position, zero speed, no feedforward torque
    sendMITCommand(R_hip_ang,  v_des, kp + 200, kd + 80, 0.0f, MOTOR_ID_LEFT_HIP);
    sendMITCommand(R_knee_ang,  v_des, kp + 200, kd + 80, 0.0f, MOTOR_ID_LEFT_KNEE);
    sendMITCommand(L_hip_ang,  v_des, kp + 200, kd + 80, 0.0f, MOTOR_ID_RIGHT_HIP);
    sendMITCommand(L_knee_ang,  v_des, kp + 200, kd + 80, 0.0f, MOTOR_ID_RIGHT_KNEE);

    // keep draining feedback so CAN FIFOs don't overflow
    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      decodeMotorFeedback(&canMsg);
    }
    
    logGaitData(LgaitIndex, RgaitIndex);

    delay(5);  // ~200 Hz hold loop
  }
}

void decodeMotorFeedback(struct can_frame *msg) {
  if (msg->can_dlc < 8) return;

  uint16_t motor_id = (uint16_t)(msg->can_id & 0xFF);   // Drive ID in low byte

  if (motor_id == MOTOR_ID_RIGHT_HIP)       memcpy(msgDataRightHip,  msg->data, 8);
  else if (motor_id == MOTOR_ID_RIGHT_KNEE) memcpy(msgDataRightKnee, msg->data, 8);
  else if (motor_id == MOTOR_ID_LEFT_KNEE)  memcpy(msgDataLeftKnee,  msg->data, 8);
  else if (motor_id == MOTOR_ID_LEFT_HIP)   memcpy(msgDataLeftHip,   msg->data, 8);
}

void logGaitData(int LgaitIndex, int RgaitIndex) {
  // Header once (now includes Elapsed_us)
  if (gait_step_counter == 0) {
    Serial.println("TimeStep,Elapsed_us,L_Gait_Index,R_Gait_Index,RH[8],RK[8],LK[8],LH[8]");
  }

  // decimate prints to reduce blocking (every 5th sample)
  if ((gait_step_counter % 5) != 0) { gait_step_counter++; return; }

  int64_t elapsed_us = esp_timer_get_time() - t0_us;  // <<< added: device-side elapsed time

  Serial.print(gait_step_counter); Serial.print(",");
  Serial.print((long long)elapsed_us); Serial.print(",");  // print 64-bit elapsed_us
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