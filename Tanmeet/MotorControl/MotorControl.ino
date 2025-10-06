#include <SPI.h>
#include <mcp2515.h>
#include <cstdint>


const int BUTTON_PIN = 15;
// assumes pull-up
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// arbitary points to define our different stages
int dataCheckpoints[2] = {51, 101};
bool justReachedCheckpoint = false;

struct can_frame canMsg;
MCP2515 mcp2515(5);

enum GaitState {

  SWING_UP,
  SWING_DOWN,
  INITIAL
};

GaitState curState = INITIAL;

// Common control params
float v_des = 0.0;
float kp = 40.0;
float kd = 2.0;
float torque_ff = 2.0;
  
#define GAIT_LENGTH 101
double L_knee[GAIT_LENGTH] = {0.064467, 0.074331, 0.093322, 0.118156, 0.145765, 0.173782, 0.200732, 0.225926, 0.249164, 0.270381, 0.289373, 0.305691, 0.318736, 0.327958, 0.333073, 0.334175, 0.331732, 0.326490, 0.319331, 0.311129, 0.302627, 0.294319, 0.286371, 0.278621, 0.270692, 0.262185, 0.252879, 0.242834, 0.232383, 0.222038, 0.212359, 0.203835, 0.196767, 0.191192, 0.186847, 0.183235, 0.179761, 0.175892, 0.171276, 0.165752, 0.159294, 0.151928, 0.143711, 0.134770, 0.125387, 0.116065, 0.107502, 0.100468, 0.095640, 0.093459, 0.094056, 0.097284, 0.102846, 0.110498, 0.120221, 0.132296, 0.147250, 0.165738, 0.188439, 0.215986, 0.248939, 0.287736, 0.332620, 0.383527, 0.439959, 0.500893, 0.564828, 0.630043, 0.694954, 0.758340, 0.819280, 0.876881, 0.930024, 0.977325, 1.017316, 1.048739, 1.070786, 1.083212, 1.086280, 1.080587, 1.066864, 1.045814, 1.018047, 0.984064, 0.944284, 0.899057, 0.848651, 0.793217, 0.732766, 0.667199, 0.596433, 0.520656, 0.440760, 0.358855, 0.278567, 0.204591, 0.141522, 0.092671, 0.059562, 0.042007, 0.038405};
double L_hip[GAIT_LENGTH] = {0.289862, 0.294106, 0.299675, 0.305515, 0.310784, 0.314967, 0.317822, 0.319227, 0.319036, 0.317008, 0.312815, 0.306110, 0.296648, 0.284423, 0.269742, 0.253211, 0.235617, 0.217761, 0.200304, 0.183673, 0.168035, 0.153324, 0.139305, 0.125649, 0.112010, 0.098116, 0.083820, 0.069120, 0.054131, 0.039038, 0.024051, 0.009396, -0.004705, -0.018052, -0.030488, -0.041909, -0.052281, -0.061665, -0.070250, -0.078398, -0.086628, -0.095527, -0.105598, -0.117101, -0.129972, -0.143842, -0.158155, -0.172330, -0.185914, -0.198666, -0.210577, -0.221803, -0.232542, -0.242877, -0.252683, -0.261621, -0.269205, -0.274901, -0.278197, -0.278642, -0.275855, -0.269508, -0.259313, -0.245034, -0.226550, -0.203951, -0.177646, -0.148385, -0.117162, -0.085016, -0.052805, -0.021057, 0.010050, 0.040595, 0.070755, 0.100649, 0.130239, 0.159319, 0.187544, 0.214489, 0.239725, 0.262917, 0.283915, 0.302772, 0.319662, 0.334727, 0.347929, 0.358969, 0.367321, 0.372359, 0.373525, 0.370504, 0.363396, 0.352850, 0.340090, 0.326739, 0.314464, 0.304605, 0.297928, 0.294529, 0.293879};


// Note we are simply using the right knee/hip data arrays
double R_knee[GAIT_LENGTH] = {0.039173, 0.059021, 0.085967, 0.115610, 0.144314, 0.169891, 0.191788, 0.210720, 0.227946, 0.244510, 0.260765, 0.276288, 0.290163, 0.301404, 0.309336, 0.313789, 0.315066, 0.313749, 0.310455, 0.305674, 0.299740, 0.292881, 0.285280, 0.277085, 0.268387, 0.259220, 0.249582, 0.239484, 0.228979, 0.218164, 0.207169, 0.196120, 0.185119, 0.174242, 0.163575, 0.153282, 0.143645, 0.135032, 0.127776, 0.122043, 0.117765, 0.114696, 0.112556, 0.111165, 0.110518, 0.110768, 0.112159, 0.114934, 0.119250, 0.125131, 0.132526, 0.141447, 0.152124, 0.165032, 0.180784, 0.199944, 0.222902, 0.249841, 0.280827, 0.315940, 0.355357, 0.399288, 0.447811, 0.500702, 0.557384, 0.616989, 0.678434, 0.740430, 0.801478, 0.859921, 0.914084, 0.962450, 1.003836, 1.037468, 1.062958, 1.080177, 1.089130, 1.089876, 1.082515, 1.067232, 1.044349, 1.014357, 0.977879, 0.935595, 0.888165, 0.836206, 0.780312, 0.721086, 0.659118, 0.594912, 0.528814, 0.461043, 0.391888, 0.322092, 0.253307, 0.188328, 0.130832, 0.084629, 0.052771, 0.036826, 0.036496};
double R_hip[GAIT_LENGTH] = {0.330710, 0.336243, 0.342093, 0.346828, 0.349355, 0.349147, 0.346295, 0.341347, 0.335008, 0.327803, 0.319869, 0.310954, 0.300598, 0.288403, 0.274250, 0.258375, 0.241274, 0.223518, 0.205571, 0.187704, 0.170029, 0.152584, 0.135401, 0.118501, 0.101854, 0.085359, 0.068890, 0.052363, 0.035805, 0.019357, 0.003225, -0.012408, -0.027455, -0.041935, -0.055917, -0.069412, -0.082317, -0.094431, -0.105567, -0.115683, -0.124958, -0.133761, -0.142530, -0.151609, -0.161121, -0.170927, -0.180683, -0.189985, -0.198526, -0.206199, -0.213080, -0.219312, -0.224949, -0.229848, -0.233664, -0.235952, -0.236298, -0.234403, -0.230068, -0.223136, -0.213446, -0.200851, -0.185285, -0.166822, -0.145656, -0.122032, -0.096169, -0.068275, -0.038644, -0.007752, 0.023762, 0.055214, 0.086032, 0.115859, 0.144572, 0.172211, 0.198860, 0.224518, 0.249010, 0.271981, 0.292979, 0.311585, 0.327532, 0.340792, 0.351604, 0.360427, 0.367799, 0.374125, 0.379469, 0.383465, 0.385413, 0.384554, 0.380432, 0.373177, 0.363612, 0.353111, 0.343296, 0.335672, 0.331301, 0.330586, 0.333183};


int LgaitIndex = 0;
int RgaitIndex = 51;



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

// setup to first position
int i = 0;
void loopInitial() {
  sendMITCommand(-(R_hip[LgaitIndex] + 0.02) * i/20,  v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_HIP);
  sendMITCommand(-(R_knee[LgaitIndex] * .8) * i/20, v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_KNEE);
  sendMITCommand((R_hip[RgaitIndex] + 0.02) * i/20, v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_HIP);
  sendMITCommand((R_knee[RgaitIndex] * .8) * i/20,v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_KNEE);
  i++;
  if (i == 20) {
    // keep at datapoint 19 until state transitions
    justReachedCheckpoint = true;
    i = 19;
  }

}

void loopData() {
    sendMITCommand(-(R_hip[LgaitIndex] + 0.02),  v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_HIP);
    sendMITCommand(-(R_knee[LgaitIndex] * .8), v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_KNEE);
    sendMITCommand((R_hip[RgaitIndex] + 0.02), v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_HIP);
    sendMITCommand((R_knee[RgaitIndex] * .8),v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_KNEE);

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
    // logic should switch between swing up and swing down states toggle between and 0 1
    curState = (GaitState)((curState + 1) % 2);
    Serial.print("Switched to state: ");
    Serial.println(curState);
    justReachedCheckpoint = true;
  }
  // 

  switch (curState) {
    case INITIAL:
      loopInitial();
      break;
    case SWING_UP:
      loopData();
      if (LgaitIndex != dataCheckpoints[0]) {
        LgaitIndex = (LgaitIndex + 1) % GAIT_LENGTH;
        RgaitIndex = (RgaitIndex + 1) % GAIT_LENGTH;    
      } else {
        justReachedCheckpoint = true;
      }
      break;

    case SWING_DOWN:
      loopData();
      if (LgaitIndex != dataCheckpoints[1]) {
        LgaitIndex = (LgaitIndex + 1) % GAIT_LENGTH;
        RgaitIndex = (RgaitIndex + 1) % GAIT_LENGTH;      
      } else {
        justReachedCheckpoint = true;
      }
      break;
  }
  // adjust delay if needed
  delay(200); // ~50 Hz control loop
}
