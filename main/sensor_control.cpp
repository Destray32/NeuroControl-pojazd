#include "sensor_control.h"

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