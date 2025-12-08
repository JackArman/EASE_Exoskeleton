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

## 🚀 Future Work

The EASE Exoskeleton team plans several improvements and expansions to the current system to enhance performance, maintainability, and research capabilities:

### 1. Refactor to State Machine Architecture
- Currently, the gait control loop is a **linear data-driven loop**, which makes it harder to extend and maintain.
- Future versions will use **object-oriented programming and state machines**:
  - Separate classes for each phase of gait (e.g., stance, swing, recovery)
  - Each class will handle its own motor commands, timing, and transitions
  - This will make the system **modular, easier to debug, and extendable** for more complex gait patterns

### 2. Adaptive Control
- Implement **adaptive or feedback-based control** to allow the exoskeleton to respond to:
  - Variations in user weight and gait speed
  - Changes in terrain or resistance
  - Motor performance or wear over time
- This could include closed-loop PID gains that automatically adjust based on sensor feedback or user input

### 3. Enhanced Safety Features
- Introduce **automatic emergency stops** if torque, velocity, or position limits are exceeded
- Add **real-time monitoring** of joint temperatures, currents, and motor health
- Improve **soft-start and soft-stop routines** to reduce mechanical stress and risk to users

### 4. Expanded Hardware Integration
- Add support for **additional sensors**, such as:
  - IMUs for detecting limb orientation
  - Force sensors under the foot to detect load and stance phase
- Allow **wireless telemetry** for remote monitoring and data logging

### 5. Data Logging and Analysis
- Improve logging format for better post-processing
- Implement **live visualization tools** to observe gait patterns and motor states in real time
- Enable **export of gait data** for research and comparison

### 6. User Customization
- Provide a **calibration routine** for different users
- Allow dynamic adjustment of gait speed, step length, and torque limits
- Include a **graphical user interface (GUI)** for simplified testing and control

### 7. Digital Twin and User Modeling

- Develop a **digital twin** of the exoskeleton and the user’s lower limbs:
  - Simulate joint positions, velocities, torques, and gait patterns in software
  - Mirror the physical system in real-time to analyze performance and predict mechanical stress
- Use **sensor data** (IMUs, force sensors, encoders) to update the digital twin:
  - Track the user’s actual walking motion
  - Compare intended gait trajectory vs. real user motion
  - Adjust motor commands dynamically for improved assistance and safety
- Enable **personalized gait adaptation**:
  - The system can learn each user’s walking style
  - Adjust torque, step length, and timing for maximum comfort and efficiency
- Facilitate **research and testing**:
  - Run simulations to test new gait algorithms without risking the physical hardware
  - Collect data for rehabilitation studies or machine learning models

These improvements will make the exoskeleton system more robust, adaptable, and suitable for both research and real-world prototyping.

---
## 👨‍💻 Project Information

EASE Exoskeleton Project  
ESP32 + MCP2515 + MIT Motor Protocol

