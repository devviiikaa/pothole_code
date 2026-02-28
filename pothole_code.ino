/************* BLYNK SETTINGS *************/
#define BLYNK_TEMPLATE_ID "TMPL3SnGpioAj"
#define BLYNK_TEMPLATE_NAME "Pothole Alert"
#define BLYNK_AUTH_TOKEN "2T5T2PQBj093DB6ihpTd084REUAb0UWe"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

/************* WIFI *************/
char ssid[] = "Devikasi";
char pass[] = "devikasoopar";

/************* OBJECTS *************/
MPU6050 mpu;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(D7, D8);

BlynkTimer timer;

/************* PINS *************/
int trig = D5;
int echo = D6;
int led = D0;
int buzzer = D3;

/************* VARIABLES *************/
long duration;
int distance;
int prevAz = 0;

bool potholeActive = false;
unsigned long lastAlertTime = 0;

bool distanceTrigger = false;
unsigned long distanceTime = 0;

/************* FUNCTIONS *************/

// Ultrasonic distance
int getDistance() {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH, 30000);
  distance = duration * 0.034 / 2;

  return distance;
}

// Buzzer patterns
void beepLow() {
  digitalWrite(buzzer, HIGH);
  delay(120);
  digitalWrite(buzzer, LOW);
}

void beepMedium() {
  for(int i=0;i<2;i++){
    digitalWrite(buzzer, HIGH);
    delay(120);
    digitalWrite(buzzer, LOW);
    delay(100);
  }
}

void beepHigh() {
  for(int i=0;i<3;i++){
    digitalWrite(buzzer, HIGH);
    delay(200);
    digitalWrite(buzzer, LOW);
    delay(100);
  }
}

// Main logic
void sendData() {

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  int shock = abs(az - prevAz);
  prevAz = az;

  int dist = getDistance();

  if (dist >= 7) {
    distanceTrigger = true;
    distanceTime = millis();
  }

  if (distanceTrigger &&
      (millis() - distanceTime < 1000) &&
      shock > 1000 &&
      !potholeActive) {

    potholeActive = true;
    lastAlertTime = millis();

    digitalWrite(led, HIGH);

    String severity;
    int severityLevel = 1;

    // ===== SEVERITY LOGIC =====
    if (shock > 4000) {
      severity = "🔴 HIGH RISK";
      severityLevel = 3;
      beepHigh();

      Blynk.logEvent("pothole_alert",
      "🚨 HIGH RISK IMPACT! Possible accident. LAT:10.0647 LON:76.6291");
    }
    else if (shock > 2000) {
      severity = "🟡 MEDIUM RISK";
      severityLevel = 2;
      beepMedium();

      Blynk.logEvent("pothole_alert",
      "⚠ MEDIUM RISK POTHOLE detected.");
    }
    else {
      severity = "🟢 LOW RISK";
      severityLevel = 1;
      beepLow();

      Blynk.logEvent("pothole_alert",
      "ℹ LOW RISK pothole detected.");
    }

    Serial.println("⚠ POTHOLE DETECTED!");
    Serial.print("Severity: ");
    Serial.println(severity);

    float lat = 10.0647;
    float lon = 76.6291;

    Serial.print("LAT: ");
    Serial.println(lat, 6);
    Serial.print("LON: ");
    Serial.println(lon, 6);

    // Website values
    Blynk.virtualWrite(V0, 1);       // pothole
    Blynk.virtualWrite(V1, severityLevel); // severity
  }

  if (millis() - distanceTime > 1000) {
    distanceTrigger = false;
  }

  if (potholeActive && millis() - lastAlertTime > 3000) {

    potholeActive = false;

    digitalWrite(led, LOW);
    digitalWrite(buzzer, LOW);

    Blynk.virtualWrite(V0, 0);
  }

  if (!potholeActive) {
    Serial.println("Road Normal");
  }

  Serial.println("-------------------");
}

/************* SETUP *************/
void setup() {

  Serial.begin(115200);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);

  Wire.begin(D2, D1);
  mpu.initialize();

  gpsSerial.begin(9600);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(500L, sendData);
}

void loop() {

  Blynk.run();
  timer.run();
}