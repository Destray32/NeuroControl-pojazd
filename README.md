# Repozytorium kodu dla pojazdu w projekcie "NeuroControl"

## Opis projektu
Projekt NeuroControl obejmuje sterowanie pojazdem przy użyciu ESP32, który tworzy własną sieć WiFi i udostępnia interfejs webowy do sterowania silnikami pojazdu.

## Komponenty sprzętowe
- ESP32 (konfiguracja: ESP32-WROOM-DA)
- Sterownik silników (H-bridge)
- Silniki DC x 2 (lewy i prawy)
- Źródło zasilania

## Podłączenia pinów
### Lewy silnik:
- ENA: pin 3 - włączenie/wyłączenie silnika
- IN1: pin 32 - kierunek obrotów 1
- IN2: pin 33 - kierunek obrotów 2

### Prawy silnik:
- ENB: pin 5 - włączenie/wyłączenie silnika
- IN3: pin 34 - kierunek obrotów 1
- IN4: pin 35 - kierunek obrotów 2

## Działanie serwera webowego
Pojazd tworzy własną sieć WiFi o nazwie "NeuroVehicle" z hasłem "neurocontrol". Po podłączeniu do sieci, serwer webowy jest dostępny pod adresem 192.168.4.1.

### Struktura serwera
- Serwer działa na porcie 80
- Oferuje prosty interfejs HTML do sterowania pojazdem
- Udostępnia następujące endpointy:
  - `/` - strona główna z przyciskami sterowania
  - `/forward` - jazda do przodu
  - `/backward` - jazda do tyłu
  - `/left` - skręt w lewo
  - `/right` - skręt w prawo
  - `/stop` - zatrzymanie pojazdu

## Funkcje sterowania silnikami
Kod zawiera następujące funkcje do sterowania silnikami:

### Podstawowe funkcje sterowania:
- `moveForward()` - włącza oba silniki do jazdy do przodu
- `moveBackward()` - włącza oba silniki do jazdy do tyłu
- `turnLeft()` - obraca pojazd w lewo (lewy silnik do tyłu, prawy do przodu)
- `turnRight()` - obraca pojazd w prawo (lewy silnik do przodu, prawy do tyłu)
- `stopMotors()` - zatrzymuje wszystkie silniki

## Konfiguracja prędkości
W kodzie zdefiniowane są dwie stałe prędkości:
- `MOTOR_SPEED = 100` - standardowa prędkość jazdy (zakres: 0-255)
- `TURN_SPEED = 80` - prędkość używana przy skręcaniu (zakres: 0-255)

## Jak użyć
1. Wgraj kod na ESP32 używając Arduino IDE
2. Podłącz zasilanie do pojazdu
3. Połącz się z siecią WiFi "NeuroVehicle" (hasło: "neurocontrol")
4. Otwórz przeglądarkę i przejdź do adresu 192.168.4.1
5. Używaj przycisków na stronie do sterowania pojazdem
