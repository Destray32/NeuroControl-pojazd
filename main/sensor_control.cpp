#include "sensor_control.h"
#include "motor_control.h"

#define SAFETY_DISTANCE 16.0 // po przekroczeniu tej wartości silniki się zatrzymują

bool isMovingForward = false;
unsigned long lastEmergencyStopTime = 0;
const unsigned long EMERGENCY_COOLDOWN = 500; // cooldown in ms to prevent rapid on/off

void setupSensor() {
    pinMode(TRIGGER_PIN, OUTPUT); // TRIGGER jako wyjście
    pinMode(ECHO_PIN, INPUT);     // ECHO jako wejście
    digitalWrite(TRIGGER_PIN, LOW); // ustawienie na starcie trigger na LOW
}

float getDistance() {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout after 30ms

    // klakulacja odległości w cm
    float distance = duration * 0.0343 / 2;

    // filtrowanie zbyt dużych wartości (np. powyżej 400cm bo to pewnie error)
    if (distance > 400 || duration == 0) {
        return -1;
    }
    
    return distance;
}

void checkObstacles() {
    unsigned long currentMillis = millis();
    
    // Only check if we're moving forward and enough time has passed since last emergency stop
    if (isMovingForward && (currentMillis - lastEmergencyStopTime > EMERGENCY_COOLDOWN)) {
        float distance = getDistance();
        
        // Valid distance reading that is too close
        if (distance > 0 && distance <= SAFETY_DISTANCE) {
            Serial.println("EMERGENCY STOP: Obstacle detected at " + String(distance) + " cm");
            stopMotors();
            isMovingForward = false;
            lastEmergencyStopTime = currentMillis;
        }
    }
}

void setForwardMovement(bool isForward) {
    isMovingForward = isForward;
}