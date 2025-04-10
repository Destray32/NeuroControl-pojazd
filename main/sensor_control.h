#ifndef SENSOR_CONTROL_H
#define SENSOR_CONTROL_H

#include <Arduino.h>

// piny dla sensora HC-SR04
#define TRIGGER_PIN 12
#define ECHO_PIN 16

void setupSensor();
float getDistance();

#endif // SENSOR_CONTROL_H