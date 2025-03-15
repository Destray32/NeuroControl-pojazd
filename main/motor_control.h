#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

// Funkcje do kontrolowania silników
void setupMotors();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
void setMotorSpeed(int leftSpeed, int rightSpeed);

#endif // MOTOR_CONTROL_H