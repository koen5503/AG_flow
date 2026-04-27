#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Macros injected from .env:
// WIFI_SSID
// WIFI_PASS
// BLYNK_AUTH_TOKEN
// CALIBRATION_FACTOR

#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "YourWiFiPassword"
#endif

#ifndef BLYNK_AUTH_TOKEN
#define BLYNK_AUTH_TOKEN "YourBlynkAuthToken"
#endif

#ifndef CALIBRATION_FACTOR
#define CALIBRATION_FACTOR 4.5
#endif

// Pins (Suitable for ESP32-C3 Super Mini, avoiding strapping pins)
const int FLOW_SENSOR_PIN = 3;
const int VALVE_OPEN_PIN = 4;
const int VALVE_CLOSE_PIN = 5;

// Global variables
volatile unsigned long pulseCount = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

BlynkTimer timer;

// Valve State Machine
enum ValveState {
    VALVE_IDLE,
    VALVE_OPENING,
    VALVE_CLOSING
};

ValveState currentValveState = VALVE_IDLE;
unsigned long valveTriggerTime = 0;
const unsigned long VALVE_PULSE_DURATION = 30; // ms

// ISR for flow sensor
void IRAM_ATTR flowSensorISR() {
    portENTER_CRITICAL_ISR(&mux);
    pulseCount++;
    portEXIT_CRITICAL_ISR(&mux);
}

// Function to trigger opening the valve
void triggerValveOpen() {
    if (currentValveState != VALVE_IDLE) return; // Prevent concurrent triggers
    Serial.println("Triggering valve OPEN...");
    digitalWrite(VALVE_OPEN_PIN, HIGH); // Assuming HIGH triggers the pulse
    currentValveState = VALVE_OPENING;
    valveTriggerTime = millis();
}

// Function to trigger closing the valve
void triggerValveClose() {
    if (currentValveState != VALVE_IDLE) return; // Prevent concurrent triggers
    Serial.println("Triggering valve CLOSE...");
    digitalWrite(VALVE_CLOSE_PIN, HIGH); // Assuming HIGH triggers the pulse
    currentValveState = VALVE_CLOSING;
    valveTriggerTime = millis();
}

// Calculate and send flow volume
void sendFlowData() {
    unsigned long currentPulseCount = 0;
    
    // Safely copy and reset pulseCount
    portENTER_CRITICAL(&mux);
    currentPulseCount = pulseCount;
    pulseCount = 0;
    portEXIT_CRITICAL(&mux);

    // Calculate liters
    // Formula: pulses / CALIBRATION_FACTOR = volume in Liters
    float liters = currentPulseCount / (float)CALIBRATION_FACTOR;

    Serial.print("Pulses in last interval: ");
    Serial.println(currentPulseCount);
    Serial.print("Calculated Volume (L): ");
    Serial.println(liters);

    // Send to Blynk Virtual Pin 1
    Blynk.virtualWrite(V1, liters);
}

// Blynk V2 listener for Valve Control
BLYNK_WRITE(V2) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        triggerValveOpen();
    } else {
        triggerValveClose();
    }
}

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    // Give Serial some time to initialize for USB CDC
    delay(2000); 
    Serial.println("Starting ESP32-C3 Water Flow & Valve Controller...");

    // Initialize Pins
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(VALVE_OPEN_PIN, OUTPUT);
    pinMode(VALVE_CLOSE_PIN, OUTPUT);
    
    digitalWrite(VALVE_OPEN_PIN, LOW);
    digitalWrite(VALVE_CLOSE_PIN, LOW);

    // Attach Interrupt
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorISR, RISING);

    // Connect to Blynk
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Setup 15-minute timer (900000 ms)
    timer.setInterval(900000L, sendFlowData);
}

void loop() {
    Blynk.run();
    timer.run();

    // Valve State Machine non-blocking logic
    if (currentValveState != VALVE_IDLE) {
        if (millis() - valveTriggerTime >= VALVE_PULSE_DURATION) {
            if (currentValveState == VALVE_OPENING) {
                digitalWrite(VALVE_OPEN_PIN, LOW);
                Serial.println("Valve OPEN pulse complete.");
            } else if (currentValveState == VALVE_CLOSING) {
                digitalWrite(VALVE_CLOSE_PIN, LOW);
                Serial.println("Valve CLOSE pulse complete.");
            }
            currentValveState = VALVE_IDLE;
        }
    }
}
