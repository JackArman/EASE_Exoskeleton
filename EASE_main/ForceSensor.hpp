#ifndef FORCESENSOR_HPP
#define FORCESENSOR_HPP

#include <Arduino.h>

// Assuming we have more than one force sensor - figure out what pins can use and put in when we instantiate the force sensor object
// #define FORCE_SENSOR_PIN 36 // ESP32 pin GPIO36 (ADC0): the FSR and 10K pulldown are connected to A0

class ForceSensor {
public:
    ForceSensor(int analogPin)
        : pin(analogPin) {}

    void begin() {
        pinMode(pin, INPUT);
        analogSetAttenuation(ADC_11db);
        // stores the baseline force reading at this point
        baseline = read();
    }

    int read() {
        return analogRead(pin);
    }

    bool forceTriggered() {
      int analogReading = read();
      // currently some arbitary value that analogRead exceeds baseline by to trigger a reading
      return (analogReading - baseline > 50);
    }

    bool relaxTriggered() {
      int analogReading = read();
      // return true when close *to some degree* to the baseline - no extra force exerted
      return (abs(analogReading - baseline) < 5);

    }
private:
    int pin;
    int baseline;
};

#endif