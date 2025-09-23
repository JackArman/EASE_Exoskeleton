#include "MotorController.hpp"

MotorController controller;

void setup() {
  controller.begin();

}

void loop() {
  controller.update();
}
