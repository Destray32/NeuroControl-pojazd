#include <WiFi.h>
#include <WebServer.h>

// Definicje pinów dla silników
#define LEFT_MOTOR_ENA 3    // Pin włączający dla lewego silnika
#define LEFT_MOTOR_IN1 32   // Pin wejściowy 1 dla lewego silnika
#define LEFT_MOTOR_IN2 33   // Pin wejściowy 2 dla lewego silnika

#define RIGHT_MOTOR_ENB 5   // Pin włączający dla prawego silnika
#define RIGHT_MOTOR_IN3 34  // Pin wejściowy 3 dla prawego silnika
#define RIGHT_MOTOR_IN4 35  // Pin wejściowy 4 dla prawego silnika

#define MOTOR_SPEED 100     // Domyślna prędkość silnika (0-255)
#define TURN_SPEED 80       // Prędkość dla skręcania (0-255)

// Ustawienia punktu dostępu WiFi
const char* ap_ssid = "NeuroVehicle";      // Nazwa sieci WiFi pojazdu
const char* ap_password = "neurocontrol";  // Hasło do sieci WiFi pojazdu

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Utworzenie serwera web na porcie 80
WebServer server(80);

void setup() {
  Serial.begin(9600);

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
  
  // Konfiguracja punktu dostępu WiFi
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  
  delay(100);
  
  Serial.println("");
  Serial.print("Punkt dostępu WiFi uruchomiony z nazwą: ");
  Serial.println(ap_ssid);
  Serial.print("Adres IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Definicja endpointów serwera
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  
  // Uruchomienie serwera
  server.begin();
  Serial.println("Serwer HTTP uruchomiony");
}

void loop() {
  // Obsługa żądań klientów
  server.handleClient();
  delay(2);  // Małe opóźnienie dla stabilności
}

// Funkcje obsługi żądań serwera
void handleRoot() {
  String html = "<html><body style='font-family: Arial; text-align: center;'>";
  html += "<h1>Sterowanie NeuroVehicle</h1>";
  html += "<div style='margin: 20px;'>";
  html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/forward'\">Do przodu</button><br>";
  html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/left'\">W lewo</button>";
  html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/stop'\">STOP</button>";
  html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/right'\">W prawo</button><br>";
  html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/backward'\">Do tyłu</button>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleForward() {
  moveForward();
  server.send(200, "text/plain", "Jazda do przodu");
}

void handleBackward() {
  moveBackward();
  server.send(200, "text/plain", "Jazda do tyłu");
}

void handleLeft() {
  turnLeft();
  server.send(200, "text/plain", "Skręt w lewo");
}

void handleRight() {
  turnRight();
  server.send(200, "text/plain", "Skręt w prawo");
}

void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "Zatrzymano");
}

// Funkcja do jazdy pojazdem do przodu
void moveForward() {
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
void moveBackward() {
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
void turnLeft() {
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
void turnRight() {
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
void stopMotors() {
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

// Opcjonalnie: Funkcja do kontrolowania prędkości obu silników
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  // Ograniczenie wartości prędkości do prawidłowego zakresu PWM (0-255)
  leftSpeed = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);
  
  if (leftSpeed > 0) {
    // Lewy silnik do przodu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, leftSpeed);
    analogWrite(LEFT_MOTOR_IN2, 0);
  } else if (leftSpeed < 0) {
    // Lewy silnik do tyłu
    digitalWrite(LEFT_MOTOR_ENA, HIGH);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, -leftSpeed);
  } else {
    // Zatrzymanie lewego silnika
    digitalWrite(LEFT_MOTOR_ENA, LOW);
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, 0);
  }
  
  if (rightSpeed > 0) {
    // Prawy silnik do przodu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, rightSpeed);
    analogWrite(RIGHT_MOTOR_IN4, 0);
  } else if (rightSpeed < 0) {
    // Prawy silnik do tyłu
    digitalWrite(RIGHT_MOTOR_ENB, HIGH);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, -rightSpeed);
  } else {
    // Zatrzymanie prawego silnika
    digitalWrite(RIGHT_MOTOR_ENB, LOW);
    analogWrite(RIGHT_MOTOR_IN3, 0);
    analogWrite(RIGHT_MOTOR_IN4, 0);
  }
}
