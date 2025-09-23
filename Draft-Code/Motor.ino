#include <SPI.h>
#include <mcp2515.h>

class Motor {
private:
  float pos = 0.0;
  float torque = 0.0;
  float velocity = 0.0;

  const float kp = 40.0;
  const float kd = 2.0;

  uint8_t motor_id;
  MCP2515* can;

  // Utility to convert float to unsigned int with bounds
  int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
    float span = x_max - x_min;
    float offset = x_min;
    if (x < x_min) x = x_min;
    if (x > x_max) x = x_max;
    return (int)((x - offset) * ((float)((1 << bits) - 1) / span));
  }

public:
  Motor(uint8_t id, MCP2515* can_interface) : motor_id(id), can(can_interface) {}

  void setPos(float p) { pos = p; }
  float getPos() const { return pos; }

  void setTorque(float t) { torque = t; }
  float getTorque() const { return torque; }

  void setVelocity(float v) { velocity = v; }
  float getVelocity() const { return velocity; }

  void sendCommand() {
    struct can_frame msg;

    int p_int  = float_to_uint(pos,     -12.56f, 12.56f, 16);
    int v_int  = float_to_uint(velocity, -33.0f, 33.0f, 12);
    int kp_int = float_to_uint(kp,       0.0f, 500.0f, 12);
    int kd_int = float_to_uint(kd,       0.0f, 5.0f,    12);
    int t_int  = float_to_uint(torque,  -65.0f, 65.0f, 12);

    uint32_t can_id = (0x08 << 8) | motor_id;
    msg.can_id = 0x80000000UL | can_id;
    msg.can_dlc = 8;

    msg.data[0] = kp_int >> 4;
    msg.data[1] = ((kp_int & 0xF) << 4) | (kd_int >> 8);
    msg.data[2] = kd_int & 0xFF;
    msg.data[3] = p_int >> 8;
    msg.data[4] = p_int & 0xFF;
    msg.data[5] = v_int >> 4;
    msg.data[6] = ((v_int & 0xF) << 4) | (t_int >> 8);
    msg.data[7] = t_int & 0xFF;

    can->sendMessage(&msg);
  }

  void emergencyStop() {
    pos = 0.0;
    velocity = 0.0;
    torque = 0.0;
    sendCommand();
  }

  String debugPrint() {
    String s = "[Motor ";
    s += String(motor_id, HEX);
    s += "] Pos: ";
    s += String(pos, 3);
    s += "  Vel: ";
    s += String(velocity, 3);
    s += "  Torque: ";
    s += String(torque, 3);
    return s;
  }
};
