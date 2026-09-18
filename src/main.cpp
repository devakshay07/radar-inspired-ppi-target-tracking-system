#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include "KalmanFilter.h"
#include "WebServer.h"

// --- CREDENTIALS ---
const char* ssid = "SmartSentry_AP";
const char* password = "YOUR_WIFI_PASSWORD"; // MUST CHANGE

// --- PINS ---
const int PIN_TRIG = 5;
const int PIN_ECHO = 34;
const int PIN_RCWL = 4;
const int PIN_SERVO_SCAN = 13; // Continuous scanning radar
const int PIN_SERVO_PAN  = 12; // Base of the targeting turret
const int PIN_SERVO_TILT = 14; // Head of the targeting turret

// --- HARDWARE ---
Servo scanServo;
Servo panServo;
Servo tiltServo;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- SYSTEM STATE ---
String currentState = "SCANNING";
KalmanFilter1D kalman(0.5, 4.0);

int currentScanAngle = 90;
int scanDirection = 1;

unsigned long lastServoMove = 0;
unsigned long lastSensorRead = 0;
unsigned long lastTelemetry = 0;

float measureDistance() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    
    long duration = pulseIn(PIN_ECHO, HIGH, 30000); // 30ms timeout
    if (duration == 0) return 400.0; // Max range if no echo
    return (duration * 0.0343) / 2.0;
}

void notifyClients() {
    StaticJsonDocument<200> doc;
    doc["state"] = currentState;
    doc["angle"] = currentScanAngle;
    doc["distance"] = kalman.getDistance();
    
    char buffer[200];
    serializeJson(doc, buffer);
    ws.textAll(buffer);
}

void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    pinMode(PIN_RCWL, INPUT);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    
    scanServo.setPeriodHertz(50);
    panServo.setPeriodHertz(50);
    tiltServo.setPeriodHertz(50);
    
    scanServo.attach(PIN_SERVO_SCAN, 500, 2400);
    panServo.attach(PIN_SERVO_PAN, 500, 2400);
    tiltServo.attach(PIN_SERVO_TILT, 500, 2400);

    scanServo.write(currentScanAngle);
    panServo.write(90);  // Center targeting turret
    tiltServo.write(90); // Level targeting turret
    
    Serial.println("\n[SYSTEM] Booting Smart Sentry...");
    WiFi.softAP(ssid, password);
    Serial.print("[WIFI] AP Started. Connect to SSID: ");
    Serial.println(ssid);
    Serial.print("[WIFI] Dashboard IP: ");
    Serial.println(WiFi.softAPIP());

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){});
    server.addHandler(&ws);
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", htmlDashboard);
    });
    
    server.begin();
    Serial.println("[SYSTEM] Web Server Running. Starting Radar Sweep...");
    delay(2000); 
}

void loop() {
    ws.cleanupClients();
    unsigned long now = millis();

    // 1. CONTINUOUS SWEEP (Scan Servo Only)
    if (now - lastServoMove >= 30) {
        currentScanAngle += scanDirection;
        if (currentScanAngle >= 160) {
            currentScanAngle = 160;
            scanDirection = -1;
        } else if (currentScanAngle <= 20) {
            currentScanAngle = 20;
            scanDirection = 1;
        }
        scanServo.write(currentScanAngle);
        lastServoMove = now;
    }

    // 2. SENSOR PINGING
    if (now - lastSensorRead >= 50) {
        bool rcwlActive = (digitalRead(PIN_RCWL) == HIGH);
        float rawDist = measureDistance();
        
        kalman.update(rawDist);
        float filteredDist = kalman.getDistance();

        // Target Logic
        if (rcwlActive && filteredDist < 150.0) {
            currentState = "THREAT_DETECTED";
            // Aim the independent targeting turret at the threat
            panServo.write(currentScanAngle);
        } else {
            currentState = "SCANNING";
        }
        
        Serial.printf("[RADAR] Angle: %03d | Raw: %6.1f cm | Filtered: %6.1f cm | RCWL: %d | %s\n", 
                      currentScanAngle, rawDist, filteredDist, rcwlActive, currentState.c_str());

        lastSensorRead = now;
    }

    // 3. TELEMETRY
    if (now - lastTelemetry >= 50) { 
        notifyClients();
        lastTelemetry = now;
    }
}
