#include <SPI.h>
#include <mcp2515.h>
#include <cstdint>


const int BUTTON_PIN = 15;
// assumes pull-up
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// arbitary points to define our different stages
int dataCheckpoints[5] = {9, 29, 49, 59, 100};
bool justReachedCheckpoint = false;

struct can_frame canMsg;
MCP2515 mcp2515(2);

/*
knee:
Initial Contact (IC)    0 – 9
Mid Stance (MS)    10 – 29
Terminal Stance (TS)    30 – 49
Pre Swing (PS)    50 – 59
Swing (SW)    60 – 99

hip:
Initial Contact (IC)    0 – 9
Mid Stance (MS)    10 – 29
Terminal Stance (TS)    30 – 49
Pre Swing (PS)    50 – 59
Swing (SW)    60 – 99

*/

enum GaitState {
  INITIAL_CONTACT,
  MID_STANCE,
  TERMINAL_STANCE,
  PRE_SWING,
  SWING,
  INITIAL
};

GaitState curState = INITIAL;

// Common control params
float v_des = 0.0;
float kp = 40.0;
float kd = 2.0;
float torque_ff = 2.0;
  
#define GAIT_LENGTH 101


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



int LgaitIndex = 0;
int RgaitIndex = 51;
int legHipOffset = 90;

float power = 


// Unique motor IDs (replace with actual IDs)
const uint8_t MOTOR_ID_LEFT_HIP = 0x03;
const uint8_t MOTOR_ID_RIGHT_HIP = 0x01;
const uint8_t MOTOR_ID_LEFT_KNEE = 0x04;
const uint8_t MOTOR_ID_RIGHT_KNEE = 0x02;

// Convert float to uint for MIT packet
int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  float offset = x_min;
  if (x < x_min) x = x_min;
  if (x > x_max) x = x_max;
  return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
}

// Send MIT-style motor command
void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
  int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
  int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
  int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
  int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
  int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

  uint32_t can_id = (0x08 << 8) | motor_id;
  canMsg.can_id = 0x80000000UL | can_id;
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
  Serial.begin(115200);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  delay(10000);
  Serial.println("Multi-joint gait tracking started");
}
void loopIdle() {
  // set into free move 

}

int getKneeOffset(int index) {
  return (index + legHipOffset) % GAIT_LENGTH;
}
// setup to first position
int i = 0;
void loopInitial() {
  int LkneeIndex = getKneeOffset(LgaitIndex);
  int RkneeIndex = getKneeOffset(RgaitIndex);
  sendMITCommand(-(R_hip[LgaitIndex] + 0.02) * i/20,  v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_HIP);
  sendMITCommand(-(R_knee[LkneeIndex] * .8) * i/20, v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_KNEE);
  sendMITCommand((R_hip[RgaitIndex] + 0.02) * i/20, v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_HIP);
  sendMITCommand((R_knee[RkneeIndex] * .8) * i/20,v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_KNEE);
  i++;
  if (i == 20) {
    // keep at datapoint 19 until state transitions
    justReachedCheckpoint = true;
    i = 19;
  }

}

void loopData() {
  int LkneeIndex = getKneeOffset(LgaitIndex);
  int RkneeIndex = getKneeOffset(RgaitIndex);
  sendMITCommand(-(R_hip[LgaitIndex] * 1.3),  v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_HIP);
  sendMITCommand(-(R_knee[LkneeIndex] * .7) * 1.3, v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_KNEE);
  sendMITCommand((R_hip[RgaitIndex] * 1.3), v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_HIP);
  sendMITCommand((R_knee[RkneeIndex] * .7) * 1.3,v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_KNEE);

}

void reachedCheckpoint(int setpoint) {
  if (LgaitIndex != dataCheckpoints[setpoint]) {
    LgaitIndex = (LgaitIndex + 1) % GAIT_LENGTH;
    RgaitIndex = (RgaitIndex + 1) % GAIT_LENGTH;   
  } else {
    justReachedCheckpoint = true;
  }
}

void loop() {
  // check signal - TODO replace with button or sensor reading
  /*
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay && reading == LOW && lastButtonState == HIGH) {
    // Cycle to next state
    currentState = (SystemState)((currentState + 1) % 3); // cycles through 3 states

  }
  lastButtonState = reading;

  below can simply add justReacheCheckpoint && buttonRead) for a combination
  
  */
  if (justReachedCheckpoint) {
    if (curState == INITIAL) {
      curState = INITIAL_CONTACT;
    } else {
      // logic should switch between swing up and swing down states toggle between and 0 1
      curState = (GaitState)((curState + 1) % 5);
    }

    Serial.print("Switched to state: ");
    Serial.println(curState);
    justReachedCheckpoint = true;
  }
  // 

/*
  INITIAL_CONTACT,
  MID_STANCE,
  TERMINAL_STANCE,
  PRE_SWING,
  SWING,
*/
  if (curState == INITIAL) {
    loopInitial();
  } else {
    loopData();
    reachedCheckpoint(curState);
  }

  // adjust delay if needed
  delay(50); // ~50 Hz control loop
}
