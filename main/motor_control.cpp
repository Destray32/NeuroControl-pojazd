#include "motor_control.h"
#include "config.h"
#include <Arduino.h>

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

    // Początkowo zatrzymaj oba silniki
    stopMotors();
}

// Funkcja do jazdy pojazdem do przodu
void moveForward()
{
    Serial.println("Jazda do przodu");

    // Lewy silnik do przodu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, MOTOR_SPEED);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Prawy silnik do przodu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, MOTOR_SPEED);
    analogWrite(RIGHT_MOTOR_IN4, 0);
}

// Funkcja do jazdy pojazdem do tyłu
void moveBackward()
{
    Serial.println("Jazda do tyłu");

    // Lewy silnik do tyłu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, MOTOR_SPEED);

    // Prawy silnik do tyłu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, MOTOR_SPEED);
}

// Funkcja do skręcania pojazdem w lewo
void turnLeft()
{
    Serial.println("Skręt w lewo");

    // Lewy silnik stop lub do tyłu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, TURN_SPEED);

    // Prawy silnik do przodu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, TURN_SPEED);
    analogWrite(RIGHT_MOTOR_IN4, 0);
}

// Funkcja do skręcania pojazdem w prawo
void turnRight()
{
    Serial.println("Skręt w prawo");

    // Lewy silnik do przodu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, TURN_SPEED);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Prawy silnik stop lub do tyłu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, TURN_SPEED);
}

// Funkcja do zatrzymania wszystkich silników
void stopMotors()
{
    Serial.println("Zatrzymanie silników");

    // Zatrzymanie lewego silnika
    digitalWrite(LEFT_MOTOR_ENA, LOW);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, 0);

    // Zatrzymanie prawego silnika
    digitalWrite(RIGHT_MOTOR_ENB, LOW);
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