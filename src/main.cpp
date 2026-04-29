// Blynk Template ID and Name MUST be defined before includes
#ifndef BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_ID "TMPL4cHUVdAmC"
#endif
#ifndef BLYNK_TEMPLATE_NAME
#define BLYNK_TEMPLATE_NAME "Beregening Doorstroming en Kraan"
#endif

#define BLYNK_PRINT Serial // Enable Blynk debug prints

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Other macros injected from .env:
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
bool isValveOpen = false; // Physical state of the bistable valve
float totalAccumulatedLiters = 0; // Total volume since boot
unsigned long lastIntervalStartTime = 0; // Timestamp of last 15-min report

// ISR for flow sensor
void IRAM_ATTR flowSensorISR() {
    portENTER_CRITICAL_ISR(&mux);
    pulseCount++;
    portEXIT_CRITICAL_ISR(&mux);
}

// Function to trigger opening the valve
void triggerValveOpen() {
    if (currentValveState != VALVE_IDLE) return; // Prevent concurrent triggers
    Serial.println(">>> COMMAND: Open Valve");
    digitalWrite(VALVE_OPEN_PIN, HIGH);
    digitalWrite(VALVE_CLOSE_PIN, LOW);
    currentValveState = VALVE_OPENING;
    valveTriggerTime = millis();
}

// Function to trigger closing the valve
void triggerValveClose() {
    if (currentValveState != VALVE_IDLE) return; // Prevent concurrent triggers
    Serial.println(">>> COMMAND: Close Valve");
    digitalWrite(VALVE_OPEN_PIN, LOW);
    digitalWrite(VALVE_CLOSE_PIN, HIGH);
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

    // Calculate liters based on factor: L/min = Hz / CALIBRATION_FACTOR
    // Volume (L) = pulses / (CALIBRATION_FACTOR * 60)
    float liters = currentPulseCount / ((float)CALIBRATION_FACTOR * 60.0);
    totalAccumulatedLiters += liters;
    lastIntervalStartTime = millis();

    Serial.println("--- 1 MIN BLYNK REPORT ---");
    Serial.print("Pulses: ");
    Serial.print(currentPulseCount);
    Serial.print(" | Liters: ");
    Serial.print(liters);
    Serial.print(" | Total: ");
    Serial.println(totalAccumulatedLiters);

    // Send to Blynk
    Blynk.virtualWrite(V1, liters);
    Blynk.virtualWrite(V3, totalAccumulatedLiters);
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
    // Initialize Serial (USB CDC on ESP32-C3 Super Mini)
    Serial.begin(115200);
    // Wait for USB CDC connection (up to 3 seconds)
    unsigned long serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart < 3000)) {
        ; // wait for USB CDC to be ready
    }
    Serial.println("========================================");
    Serial.println("  ESP32-C3 Water Flow & Valve Controller");
    Serial.println("========================================");

    // Initialize Pins
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(VALVE_OPEN_PIN, OUTPUT);
    pinMode(VALVE_CLOSE_PIN, OUTPUT);
    
    digitalWrite(VALVE_OPEN_PIN, LOW);
    digitalWrite(VALVE_CLOSE_PIN, LOW);

    // Attach Interrupt
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorISR, RISING);

    // Connect to WiFi (non-blocking approach)
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // Wait up to 10 seconds for WiFi
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 10000)) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WARNING: WiFi not connected! Will keep retrying...");
    }

    // Use Blynk.begin as recommended for standard initialization
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Setup 1-minute timer for Blynk
    timer.setInterval(60000L, sendFlowData);
    lastIntervalStartTime = millis();
    
    // Heartbeat every 15 seconds to show active status and detailed flow
    timer.setInterval(15000L, []() {
        unsigned long currentPulses = 0;
        portENTER_CRITICAL(&mux);
        currentPulses = pulseCount;
        portEXIT_CRITICAL(&mux);

        float currentIntervalLiters = currentPulses / ((float)CALIBRATION_FACTOR * 60.0);
        float totalSoFar = totalAccumulatedLiters + currentIntervalLiters;
        
        float elapsedMinutes = (millis() - lastIntervalStartTime) / 60000.0;
        float avgFlowRate = (elapsedMinutes > 0.1) ? (currentIntervalLiters / elapsedMinutes) : 0;

        Serial.print("[Heartbeat] Uptime: ");
        Serial.print(millis() / 1000);
        Serial.print("s | Valve: ");
        Serial.print(isValveOpen ? "OPEN" : "CLOSED");
        Serial.print(" | Total Flow: ");
        Serial.print(totalSoFar, 3);
        Serial.print(" L | Avg Flow (1m period): ");
        Serial.print(avgFlowRate, 3);
        Serial.println(" L/min");
    });
    
    Serial.println("Setup complete. Running main loop...");
}

void loop() {
    Blynk.run();
    timer.run();

    // Valve State Machine non-blocking logic
    if (currentValveState != VALVE_IDLE) {
        if (millis() - valveTriggerTime >= VALVE_PULSE_DURATION) {
            if (currentValveState == VALVE_OPENING) {
                digitalWrite(VALVE_OPEN_PIN, LOW);
                isValveOpen = true;
                Serial.println(">>> Valve is now OPEN (pulse complete)");
            } else if (currentValveState == VALVE_CLOSING) {
                digitalWrite(VALVE_CLOSE_PIN, LOW);
                isValveOpen = false;
                Serial.println(">>> Valve is now CLOSED (pulse complete)");
            }
            currentValveState = VALVE_IDLE;
        }
    }
}
