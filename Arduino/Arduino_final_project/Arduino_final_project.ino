#include <WiFiS3.h>
#include "ThingSpeak.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <LiquidCrystal.h>
#include <Servo.h>

// --- WIFI ---
char ssid[] = "Galaxy A54 5G CA46";
char pass[] = "445648vkvnstcsq";

// --- THINGSPEAK ---
unsigned long myChannelNumber = 3312690;
const char myWriteAPIKey[] = "90NUYDAQV29YM9ZD";

// --- TMP36 ---
#define TMP36_PIN A0

// --- LEDs ---
#define LED_AZUL A1
#define LED_ROJO A2

// --- SERVO SG90 ---
#define SERVO_PIN 6
Servo servoCierre;
bool cajaAbierta = false;

// --- LCD PARALELA (RS, E, D4, D5, D6, D7) ---
LiquidCrystal lcd(8, 7, 5, 4, 3, 2);

// --- RFID ---
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// --- TSL2561 ---
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

WiFiClient client;

// UID permitido
String uidPermitido = "F3B6C00C";

// Control envío
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 20000;

// Rangos simulados
const float TEMP_MIN_OK = 18.0;
const float TEMP_MAX_OK = 30.0;
const float TEMP_MAX_WARNING = 35.0;

void setup() {
  Serial.begin(9600);

  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_ROJO, LOW);

  // --- SERVO ---
  servoCierre.attach(SERVO_PIN);
  servoCierre.write(0);   // Empieza cerrado

  // --- LCD ---
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Iniciando...");

  // --- RFID ---
  SPI.begin();
  rfid.PCD_Init();

  // --- I2C / TSL2561 ---
  Wire.begin();

  if (!tsl.begin()) {
    Serial.println("Error TSL2561");
    lcd.clear();
    lcd.print("Error TSL2561");
    delay(1000);
  } else {
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  }

  // --- ThingSpeak ---
  ThingSpeak.begin(client);
  conectarWiFi();

  lcd.clear();
  lcd.print("Sistema listo");
  lcd.setCursor(0, 1);
  lcd.print("No Card");
  delay(1500);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // --- TEMPERATURA TMP36 ---
  int lectura = analogRead(TMP36_PIN);
  float voltaje = lectura * (5.0 / 1023.0);
  float temperatura = (voltaje - 0.5) * 100.0;

  String estadoTemp;
  int estadoTempNum;

  if (temperatura >= TEMP_MIN_OK && temperatura <= TEMP_MAX_OK) {
    estadoTemp = "OK";
    estadoTempNum = 1;

    digitalWrite(LED_AZUL, HIGH);
    digitalWrite(LED_ROJO, LOW);
  }
  else if (temperatura > TEMP_MAX_OK && temperatura <= TEMP_MAX_WARNING) {
    estadoTemp = "AVISO";
    estadoTempNum = 2;

    digitalWrite(LED_AZUL, LOW);
    digitalWrite(LED_ROJO, HIGH);
    delay(150);
    digitalWrite(LED_ROJO, LOW);
    delay(150);
  }
  else {
    estadoTemp = "PELIGRO";
    estadoTempNum = 0;

    digitalWrite(LED_AZUL, LOW);
    digitalWrite(LED_ROJO, HIGH);
  }

  // --- LUZ TSL2561 ---
  sensors_event_t event;
  tsl.getEvent(&event);

  float luz = 0;
  if (event.light) {
    luz = event.light;
  }

  // --- RFID ---
  String estado = "NO CARD";
  int tarjetaDetectada = 0;
  int accesoNum = -1;
  int cajaEstadoNum = cajaAbierta ? 1 : 0;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    tarjetaDetectada = 1;

    String uid = "";

    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Detected Card");

    if (uid == uidPermitido) {
      estado = "ACCEPT";
      accesoNum = 1;

      if (cajaAbierta == false) {
        servoCierre.write(90);
        cajaAbierta = true;
        cajaEstadoNum = 1;

        lcd.setCursor(0, 1);
        lcd.print("Accepted Open");
      } else {
        servoCierre.write(0);
        cajaAbierta = false;
        cajaEstadoNum = 0;

        lcd.setCursor(0, 1);
        lcd.print("Accepted Close");
      }
    } else {
      estado = "DENIED";
      accesoNum = 0;

      lcd.setCursor(0, 1);
      lcd.print("Denied");
    }

    delay(1500);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  else {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temperatura, 1);
    lcd.print(" ");
    lcd.print(estadoTemp);

    lcd.setCursor(0, 1);
    lcd.print("No Card ");
    if (cajaAbierta) {
      lcd.print("Open");
    } else {
      lcd.print("Closed");
    }
  }

  // --- SERIAL ---
  Serial.print("Temp: ");
  Serial.print(temperatura);
  Serial.print(" C | Luz: ");
  Serial.print(luz);
  Serial.print(" lux | RFID: ");
  Serial.print(estado);
  Serial.print(" | Caja: ");
  Serial.println(cajaAbierta ? "ABIERTA" : "CERRADA");
  Serial.print("DATA,");
Serial.print(temperatura);
Serial.print(",");
Serial.print(luz);
Serial.print(",");
Serial.print(tarjetaDetectada);
Serial.print(",");
Serial.print(accesoNum);
Serial.print(",");
Serial.print(estadoTempNum);
Serial.print(",");
Serial.println(cajaEstadoNum);

  // --- THINGSPEAK ---
  if (millis() - ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();

    ThingSpeak.setField(1, temperatura);
    ThingSpeak.setField(2, luz);
    ThingSpeak.setField(3, tarjetaDetectada);
    ThingSpeak.setField(4, accesoNum);
    ThingSpeak.setField(5, estadoTempNum);
    ThingSpeak.setField(6, cajaEstadoNum);

    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if (x == 200) {
      Serial.println("Datos enviados a ThingSpeak.");
    } else {
      Serial.println("Error HTTP: " + String(x));
    }
  }

  delay(500);
}

void conectarWiFi() {
  Serial.print("Conectando WiFi...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(5000);
  }

  Serial.println();
  Serial.println("WiFi conectado.");

  lcd.clear();
  lcd.print("WiFi conectado");
  delay(1000);
}