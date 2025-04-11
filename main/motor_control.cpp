#include "motor_control.h"
#include "sensor_control.h"
#include "config.h"
#include <Arduino.h>

extern int getCurrentSpeedValue(); // Deklaracja funkcji z innego pliku

// Inicjalizacja pinów silników
void setupMotors()
{
    // Konfiguracja pinów lewego silnika
    pinMode(LEFT_MOTOR_ENA, OUTPUT);
    pinMode(LEFT_MOTOR_IN1, OUTPUT);
    pinMode(LEFT_MOTOR_IN2, OUTPUT);

    // Konfiguracja pinów prawego silnika
    pinMode(RIGHT_MOTOR_ENB, OUTPUT);
    pinMode(RIGHT_MOTOR_IN3, OUTPUT);
    pinMode(RIGHT_MOTOR_IN4, OUTPUT);

}

// Funkcja do jazdy pojazdem do przodu
void moveForward()
{
    setForwardMovement(true);
    // sprawdzenie dystantu do przeszkody z przodu
    float distance = getDistance();

    if (distance <= 16.0 && distance > 0.0)
    {
        Serial.println("Zatrzymanie silników z powodu przeszkody z przodu (dystans: " + String(distance) + ")");
        stopMotors();
        return;
    }

    int currentSpeed = getCurrentSpeedValue();
    Serial.println("Jazda do przodu (prędkość: " + String(currentSpeed) + ")");

    // Lewy silnik do przodu
    // digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, currentSpeed);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Prawy silnik do przodu
    // digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, currentSpeed);
}

// Funkcja do jazdy pojazdem do tyłu
void moveBackward()
{
    setForwardMovement(false);
    int currentSpeed = getCurrentSpeedValue();
    Serial.println("Jazda do tyłu (prędkość: " + String(currentSpeed) + ")");

    analogWrite(RIGHT_MOTOR_IN3, currentSpeed);
    analogWrite(RIGHT_MOTOR_IN4, 0);

    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, currentSpeed);
}

// Funkcja do skręcania pojazdem w lewo
void turnLeft()
{
    setForwardMovement(false);
    // Dla skrętów używamy 80% aktualnej prędkości
    int currentSpeed = getCurrentSpeedValue() * 0.8;
    Serial.println("Skręt w lewo (prędkość: " + String(currentSpeed) + ")");

    // Lewy silnik stop lub do tyłu
    // digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, currentSpeed);

    // Prawy silnik do przodu
    // digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, currentSpeed);
}

// Funkcja do skręcania pojazdem w prawo
void turnRight()
{
    setForwardMovement(false);
    // Dla skrętów używamy 80% aktualnej prędkości
    int currentSpeed = getCurrentSpeedValue() * 0.8;
    Serial.println("Skręt w prawo (prędkość: " + String(currentSpeed) + ")");

    // Lewy silnik do przodu
    // digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, currentSpeed);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Prawy silnik stop lub do tyłu
    // digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, currentSpeed);
    analogWrite(RIGHT_MOTOR_IN4, 0);
}

// Funkcja do zatrzymania wszystkich silników
void stopMotors()
{
    setForwardMovement(false);
    Serial.println("Zatrzymanie silników");

    // Zatrzymanie lewego silnika
    // digitalWrite(LEFT_MOTOR_ENA, LOW);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Zatrzymanie prawego silnika
    // digitalWrite(RIGHT_MOTOR_ENB, LOW);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, 0);
}

// Funkcja do kontrolowania prędkości obu silników
void setMotorSpeed(int leftSpeed, int rightSpeed)
{
    // Ograniczenie wartości prędkości do prawidłowego zakresu PWM (0-255)
    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    if (leftSpeed > 0)
    {
        // Lewy silnik do przodu
        digitalWrite(LEFT_MOTOR_ENA, HIGH);
        analogWrite(LEFT_MOTOR_IN1, leftSpeed);
        analogWrite(LEFT_MOTOR_IN2, 0);
    }
    else if (leftSpeed < 0)
    {
        // Lewy silnik do tyłu
        digitalWrite(LEFT_MOTOR_ENA, HIGH);
        analogWrite(LEFT_MOTOR_IN1, 0);
        analogWrite(LEFT_MOTOR_IN2, -leftSpeed);
    }
    else
    {
        // Zatrzymanie lewego silnika
        digitalWrite(LEFT_MOTOR_ENA, LOW);
        analogWrite(LEFT_MOTOR_IN1, 0);
        analogWrite(LEFT_MOTOR_IN2, 0);
    }

    if (rightSpeed > 0)
    {
        // Prawy silnik do przodu
        digitalWrite(RIGHT_MOTOR_ENB, HIGH);
        analogWrite(RIGHT_MOTOR_IN3, rightSpeed);
        analogWrite(RIGHT_MOTOR_IN4, 0);
    }
    else if (rightSpeed < 0)
    {
        // Prawy silnik do tyłu
        digitalWrite(RIGHT_MOTOR_ENB, HIGH);
        analogWrite(RIGHT_MOTOR_IN3, 0);
        analogWrite(RIGHT_MOTOR_IN4, -rightSpeed);
    }
    else
    {
        // Zatrzymanie prawego silnika
        digitalWrite(RIGHT_MOTOR_ENB, LOW);
        analogWrite(RIGHT_MOTOR_IN3, 0);
        analogWrite(RIGHT_MOTOR_IN4, 0);
    }
}