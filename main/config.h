#ifndef CONFIG_H
#define CONFIG_H

// Definicje pinów dla silników
#define LEFT_MOTOR_ENA 2    // Pin włączający dla lewego silnika
#define LEFT_MOTOR_IN1 32   // Pin wejściowy 1 dla lewego silnika
#define LEFT_MOTOR_IN2 33   // Pin wejściowy 2 dla lewego silnika

#define RIGHT_MOTOR_ENB 5   // Pin włączający dla prawego silnika
#define RIGHT_MOTOR_IN3 25  // Pin wejściowy 3 dla prawego silnika
#define RIGHT_MOTOR_IN4 26  // Pin wejściowy 4 dla prawego silnika

// tryby prędkości
#define SPEED_MODE_LOW 60      // Prędkość w trybie niskim
#define SPEED_MODE_NORMAL 100  // Prędkość w trybie normalnym (domyślna)
#define SPEED_MODE_HIGH 150    // Prędkość w trybie wysokim

#define MOTOR_SPEED 100     // Domyślna prędkość silnika (0-255)
#define TURN_SPEED 80       // Prędkość dla skręcania (0-255)

// Ustawienia punktu dostępu WiFi
#define AP_SSID "NeuroVehicle"      // Nazwa sieci WiFi pojazdu
#define AP_PASSWORD "neurocontrol"  // Hasło do sieci WiFi pojazdu

#define LOCAL_IP_ADDRESS 192,168,4,1
#define GATEWAY_ADDRESS 192,168,4,1
#define SUBNET_MASK 255,255,255,0

#endif // CONFIG_H