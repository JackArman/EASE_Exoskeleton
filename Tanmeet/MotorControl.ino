// // // #include <SPI.h>
// // // #include <mcp2515.h>

// // // struct can_frame canMsg;
// // // MCP2515 mcp2515(5);

// // // // Gait cycle hip angles (radians, ~0–100% gait cycle)
// // // // Sampled from typical sagittal hip flexion-extension profile
// // // float gaitData[] = {
// // //     0.307191696, 0.320702597, 0.335723251, 0.350966037, 0.365628158, 0.379334908, 0.391977391,
// // //     0.403520446, 0.413852649, 0.422730594, 0.429827527, 0.434845055, 0.437627929, 0.438227203,
// // //     0.436898228, 0.434046333, 0.430151173, 0.425689167, 0.421063756, 0.41654367, 0.412219626,
// // //     0.408003419, 0.403689433, 0.399061424, 0.393998356, 0.388533416, 0.382847828, 0.377219404,
// // //     0.371953561, 0.367315923, 0.363470992, 0.360437629, 0.358073758, 0.356108859, 0.354218565,
// // //     0.352114014, 0.349602545, 0.346597338, 0.343083466, 0.339076288, 0.334605936, 0.329741218,
// // //     0.324636475, 0.319565118, 0.314906373, 0.311079509, 0.308452884, 0.307266283, 0.307591267,
// // //     0.309347274, 0.312373349, 0.316536437, 0.3218264, 0.328395731, 0.336531098, 0.34658972,
// // //     0.358939819, 0.373926738, 0.39185461, 0.412962421, 0.437381501, 0.46507719, 0.495778572,
// // //     0.528929588, 0.56371351, 0.599193599, 0.634507974, 0.668992761, 0.702147184, 0.733484913,
// // //     0.762397478, 0.788131048, 0.809888132, 0.826983568, 0.838978467, 0.845738828, 0.847407798,
// // //     0.844310707, 0.836844472, 0.825392719, 0.810285893, 0.791797457, 0.770155346, 0.74554977,
// // //     0.718126608, 0.687968003, 0.655079803, 0.619408207, 0.580907836, 0.539681668, 0.496214447,
// // //     0.451654425, 0.407973613, 0.367727353, 0.333414805, 0.306837898, 0.288824631, 0.279273928,
// // //     0.277314439
// // // };
// // // int gaitLength = sizeof(gaitData) / sizeof(gaitData[0]);
// // // int gaitIndex = 0;

// // // // Float to uint conversion
// // // int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
// // //   float span = x_max - x_min;
// // //   float offset = x_min;
// // //   if (x < x_min) x = x_min;
// // //   if (x > x_max) x = x_max;
// // //   return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
// // // }

// // // // Send MIT control packet
// // // void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
// // //   int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
// // //   int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
// // //   int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
// // //   int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
// // //   int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

// // //   uint32_t can_id = (0x08 << 8) | motor_id;
// // //   canMsg.can_id = 0x80000000UL | can_id;
// // //   canMsg.can_dlc = 8;

// // //   canMsg.data[0] = kp_int >> 4;
// // //   canMsg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
// // //   canMsg.data[2] = kd_int & 0xFF;
// // //   canMsg.data[3] = p_int >> 8;
// // //   canMsg.data[4] = p_int & 0xFF;
// // //   canMsg.data[5] = v_int >> 4;
// // //   canMsg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
// // //   canMsg.data[7] = t_int & 0xFF;

// // //   mcp2515.sendMessage(&canMsg);
// // // }

// // // void setup() {
// // //   Serial.begin(115200);
// // //   SPI.begin();

// // //   mcp2515.reset();
// // //   mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
// // //   mcp2515.setNormalMode();P51322,

#include <SPI.h>
#include <mcp2515.h>
#include <cstdint>
struct can_frame canMsg;
float hipLeftGait[] = { 0.124173751, 0.118893376, 0.113848102, 0.109173964, 0.105117433,
0.102018233, 0.100256832, 0.100186874, 0.102080963, 0.106106823,
0.11232985, 0.120733988, 0.131244553, 0.143739973, 0.158043273,
0.173911082, 0.191038232, 0.209083343, 0.227698099, 0.246550355,
0.265338018, 0.283799304, 0.301721094, 0.318948488, 0.335391974,
0.351023091, 0.365848232, 0.379866243, 0.393034221, 0.405265861,
0.41645958, 0.426528165, 0.43540335, 0.443013095, 0.449250316,
0.453956445, 0.456932749, 0.457979636, 0.456954662, 0.4538404,
0.448814491, 0.442309599, 0.435024735, 0.427844811, 0.421673648,
0.417247725, 0.414994627, 0.414963804, 0.416826652};

float hipRightGait[] = { 0.444408834, 0.446378444, 0.447181602, 0.446534546, 
0.444555754, 0.44167249, 0.438403041, 0.435104709, 0.431803519, 0.428186508, 
0.423750571, 0.418031449, 0.410801834, 0.402159281, 0.392475268, 0.382249264, 
0.371940883, 0.361855035, 0.352108682, 0.342670401, 0.333435109, 0.324296727, 
0.315192117, 0.306114271, 0.29710261, 0.288223599, 0.279548379, 0.271135017, 
0.263022213, 0.25523342, 0.247781926, 0.240668297, 0.233876164, 0.227373995, 
0.221124361, 0.215093483, 0.209257371, 0.203601548, 0.19811274, 0.192762414, 
0.187491954, 0.182216295, 0.176855445, 0.171379818, 0.165841156, 0.160361801, 
0.155083568, 0.150103835, 0.145434597, 0.141005461, 0.1367128, 0.132496006, 
0.128411569, 0.124674405, 0.121651278, 0.119807204, 0.119622431, 0.121501346, 
0.125700023, 0.132290417, 0.14116961, 0.152102437, 0.164782903, 0.178896222, 
0.194161315, 0.210338075, 0.227205111, 0.244534383, 0.262087803, 0.2796385, 
0.296995705, 0.314013691, 0.330585374, 0.346634911, 0.3621156, 0.377003142, 
0.391271264, 0.404852681, 0.417607305, 0.429325209, 0.439774511, 0.448778056, 
0.456278693, 0.462354773, 0.467167722, 0.470865743, 0.473491195, 0.474939948, 
0.474992229, 0.473409571, 0.470065511, 0.465065706, 0.458813434, 0.451990084, 
0.445444401, 0.440014358, 0.436336164, 0.434702119, 0.435005202};

float kneeLeftGait[] = { 0.293922677, 0.308415849, 0.323563743, 0.338261082, 0.351931327,
0.364414184, 0.37576485, 0.38604935, 0.395212261, 0.40305695,
0.409316254, 0.413756137, 0.416258744, 0.416855181, 0.415709541,
0.413076997, 0.409267058, 0.404624974, 0.399515454, 0.394285656,
0.389207288, 0.384423983, 0.379934661, 0.375625241, 0.371339165,
0.366959236, 0.362468149, 0.357965953, 0.353642807, 0.349720093,
0.346382432, 0.343725231, 0.341733759, 0.340295787, 0.339235638,
0.338353037, 0.33745033, 0.336344883, 0.334875719, 0.33292067,
0.330426597, 0.327441818, 0.324133795, 0.320780042, 0.317728318,
0.315333481, 0.313889379, 0.313579808, 0.314466543, 0.316516612,
0.319655563, 0.323831542, 0.329076222, 0.335548785, 0.343545278,
0.353467718, 0.36576555, 0.380873062, 0.399154765, 0.420858093,
0.446061921, 0.474619878, 0.506109066, 0.539829581, 0.574900073,
0.610442321, 0.645747011, 0.680306384, 0.713692708, 0.745382196,
0.774653349, 0.800631175, 0.822443481, 0.839398729, 0.851103849,
0.857487461, 0.858735368, 0.855178714, 0.847183364, 0.835077422,
0.819129073, 0.79956465, 0.776597132, 0.750433706, 0.721248029,
0.689137589, 0.65410913, 0.616129041, 0.575240953, 0.531730545,
0.48631504, 0.440323946, 0.395751433, 0.355018568, 0.320468742,
0.293889697, 0.276271096, 0.267749361, 0.267613276};

float kneeRightGait[] = {0.303732502, 0.318647517, 0.333480779, 0.346822298, 0.358243285,
0.368154717, 0.377387544, 0.386690556, 0.396356197, 0.406109777, 0.415257546, 0.422983922, 
0.42864897, 0.431979369, 0.433100446, 0.432431158, 0.430501166, 0.427777877, 0.4245635, 
0.420982838, 0.417035979, 0.412675942, 0.407875381, 0.402664439, 0.397134494, 0.391419379, 
0.385671325, 0.380043586, 0.374677203, 0.369678617, 0.365087016, 0.360850586, 0.356843717, 
0.352921151, 0.348975502, 0.344965656, 0.340920427, 0.336935835, 0.333174712, 0.329855254, 
0.327207246, 0.325398426, 0.324470127, 0.324333704, 0.324836253, 0.325850747, 0.327329746, 
0.32930561, 0.331861743, 0.335112395, 0.339206226, 0.344341628, 0.350777205, 0.358827953, 
0.368848235, 0.381203877, 0.396240881, 0.414250028, 0.435427411, 0.45983105, 0.487344589, 
0.517658649, 0.550276421, 0.584544419, 0.619705335, 0.654963198, 0.689533962, 0.722664235, 
0.75363382, 0.781771712, 0.806494078, 0.827341331, 0.843991926, 0.856246508, 0.863995527,
 0.867188291, 0.86581687, 0.859921496, 0.84961116, 0.83508245, 0.816614714, 0.794538212, 
 0.769194496, 0.740914169, 0.710018537, 0.676836816, 0.641719979, 0.605031643, 0.56711349, 
 0.528256892, 0.488737301, 0.448964013, 0.409728228, 0.372412603, 0.338966899, 0.311556523, 
 0.292022651, 0.281398498, 0.27963726};

// Assumes all gait arrays are the same length for simplicity
int gaitLength = sizeof(hipLeftGait) / sizeof(hipLeftGait[0]);
int gaitIndex = 0;

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

void loop() {
  // Common control params
  float v_des = 0.0;
  float kp = 40.0;
  float kd = 2.0;
  float torque_ff = 0.0;
  
  delay(10);
  // Send commands to all motors
  sendMITCommand(hipLeftGait[gaitIndex],  v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_HIP);
  sendMITCommand(hipRightGait[gaitIndex], v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_HIP);
  sendMITCommand(kneeLeftGait[gaitIndex], v_des, kp, kd, torque_ff, MOTOR_ID_LEFT_KNEE);
  sendMITCommand(kneeRightGait[gaitIndex],v_des, kp, kd, torque_ff, MOTOR_ID_RIGHT_KNEE);

  gaitIndex = (gaitIndex + 1) % gaitLength;
  delay(20); // ~50 Hz control loop
}


// #include <SPI.h>
// #include <mcp2515.h>

// struct can_frame canMsg;
// MCP2515 mcp2515(5); // CS pin

// // Converts float to unsigned int in range
// int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
//   float span = x_max - x_min;
//   if (x < x_min) x = x_min;
//   if (x > x_max) x = x_max;
//   return (int)((x - x_min) * ((float)((1 << bits) - 1) / span));
// }

// void sendMITCommand(float p_des, float v_des, float kp, float kd, float t_ff, uint8_t motor_id) {
//   int p_int  = float_to_uint(p_des, -12.56f, 12.56f, 16);
//   int v_int  = float_to_uint(v_des, -33.0f, 33.0f, 12);
//   int kp_int = float_to_uint(kp, 0.0f, 500.0f, 12);
//   int kd_int = float_to_uint(kd, 0.0f, 5.0f, 12);
//   int t_int  = float_to_uint(t_ff, -65.0f, 65.0f, 12);

//   uint32_t can_id = (0x08 << 8) | motor_id;
//   canMsg.can_id = 0x80000000UL | can_id;
//   canMsg.can_dlc = 8;

//   canMsg.data[0] = kp_int >> 4;
//   canMsg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
//   canMsg.data[2] = kd_int & 0xFF;
//   canMsg.data[3] = p_int >> 8;
//   canMsg.data[4] = p_int & 0xFF;
//   canMsg.data[5] = v_int >> 4;
//   canMsg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
//   canMsg.data[7] = t_int & 0xFF;

//   mcp2515.sendMessage(&canMsg);
// }


// void setup() {
//   Serial.begin(115200);
//   SPI.begin();

//   mcp2515.reset();
//   mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);  // Match your CAN speed and crystal
//   mcp2515.setNormalMode();

//   Serial.println("CAN sniffer started...");
//   Serial.println("Sending ±6 degree test command");

//   float angle_radians = 6.0 * PI / 180.0; // 6 degrees → radians = 0.1047
//   float v_des = 0.0;
//   float kp = 40.0;
//   float kd = 2.0;
//   float torque_ff = 0.0;
//   uint8_t motor_id = 0x68; // change to your motor ID

//   sendMITCommand(angle_radians, v_des, kp, kd, torque_ff, motor_id);

//   delay(1000); // wait 1s

//   sendMITCommand(-angle_radians, v_des, kp, kd, torque_ff, motor_id);

//   Serial.println("Done.");
// }

// void loop() {
//   if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
//     // Extract standard CAN ID (ignore extended flag bits)
//     uint32_t can_id = canMsg.can_id & 0x1FFFFFFF;

//     Serial.print("CAN ID: 0x");
//     Serial.print(can_id, HEX);
//     Serial.print(" | Data: ");

//     for (int i = 0; i < canMsg.can_dlc; i++) {
//       if (canMsg.data[i] < 0x10) Serial.print("0");
//       Serial.print(canMsg.data[i], HEX);
//       Serial.print(" ");
//     }

//     Serial.println();
//   }
// }


