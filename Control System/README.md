# EASE Exoskeleton – MIT Position Control System

This project controls a powered lower-limb exoskeleton using a predefined gait pattern and a CAN-based MIT motor control protocol.  
It runs on an ESP32 with an MCP2515 CAN controller and drives four motors:

- Left Hip  
- Left Knee  
- Right Hip  
- Right Knee  

The system is designed for research and educational use.

---

## 🧠 Project Idea

The goal of this project is to simulate human walking (gait) by:

- Using prerecorded joint angle trajectories  
- Sending real-time motor commands over the CAN bus  
- Synchronising hip and knee movement  
- Allowing the user to control walking speed using a dial (potentiometer)  

This project enables work in:

- Gait research  
- Lower-limb exoskeleton control  
- Rehabilitation system prototyping  

---

## ⚙️ How the System Works

### Gait Trajectories

The system uses four lookup tables:

- `L_hip[]`  
- `L_knee[]`  
- `R_hip[]`  
- `R_knee[]`  

Each array contains 100 position values that represent one full walking step for each joint.

### Control Loop

The controller continuously:

1. Reads the current gait index  
2. Looks up the target joint angles  
3. Converts values into MIT motor protocol commands  
4. Sends the commands over the CAN bus  

This produces smooth and continuous walking motion.

### Speed Control Dial

A potentiometer (dial) connected to the ESP32 controls walking speed:

| Dial Position | Behaviour |
|---------------|----------|
| Low | Slow walking |
| Medium | Normal walking |
| High | Fast walking |
| Off | Stops motion |

Note: 
  - The potentiometer also acts as the off switch for the exoskelton
  - The current code forces `DIAL_MEDIUM` for testing purposes.

### Soft Start Movement

When the system starts, the legs are slowly moved into the initial gait position to avoid sudden or unsafe motion.

---

## 🪛 Hardware Requirements

- ESP32 development board  
- MCP2515 CAN controller  
- CAN transceiver module  
- 4 × Cubemars motors  
- RUBIK LINK V3.0 interface board  
- Potentiometer (speed control dial)  
- 24V power supply suitable for the motors  

---

## 🔌 How to Physically Connect the System (Test Setup)

Motor LED indicators:

- **Blue LED** → Power is available and the motor is ready for testing  
- **Red LED** → Hardware fault detected. It is recommended to stop and check the motor manual  

To run trial simulations using the CubeMars software:

1. Connect the **Cubemars motors** to an appropriate power supply  
2. Connect the motors to the **RUBIK LINK V3.0** board and connect the RUBIK LINK V3.0 to your laptop  
   - The RUBIK LINK V3.0 can also be used to reset the Motor ID if required  
3. Use the CubeMars software to test (INSERT LINK):
   - Velocity  
   - Power  
   - Torque  

Note: Depending on your laptop configuration, you may need to install additional USB or serial drivers so that your computer can detect the ESP32 correctly.

---

## 🧾 How to Upload the Code and Test (Arduino IDE)

## 📦 Required Libraries (IMPORTANT FOR STUDENTS)

Important: Students must install the MCP2515 library before the code will compile.

The required ZIP file is located in:

Control System/MIT_position_control/Arduino MCP2515 Library.zip

### Setup and Upload Steps

1. Connect the motors to the required **24V power supply**  
2. Ensure the ESP32 board is correctly connected to:
   - The MCP2515 CAN module  
   - The motor CAN wiring  
3. Connect the ESP32 board to your laptop using a **USB-C cable**  
4. Open **Arduino IDE**  
5. Select:
   - Board: `ESP32 Dev Module`  
   - The correct COM/Serial Port  

   Note: Depending on laptop configuration, some students may need to install additional drivers to allow the laptop to detect the ESP32 as a serial device.

6. Click **Upload** in Arduino IDE to flash the code to the ESP32  

Once uploaded, the motors will begin running the code

---

## 📊 Serial Monitoring (Optional)

The system can output real-time data over the Serial Monitor.

Recommended settings:

- Baud rate: `921600`  

Data includes gait step indices and raw motor feedback for debugging and analysis.

---

## ⚠️ Safety Notice

This system controls high-power motors.

Do NOT:

- Run the system while being worn  
- Touch moving parts while powered  
- Operate without supervision  

Always use physical safety stops and testing rigs when experimenting.

This system is for **research and educational use only**.

---

## 👨‍💻 Project Information

EASE Exoskeleton Project  
ESP32 + MCP2515 + MIT Motor Protocol
