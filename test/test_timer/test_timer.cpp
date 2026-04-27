#include <unity.h>

// Mocking Arduino functions for native test
unsigned long mocked_millis = 0;
int pin_states[10] = {0};

void digitalWrite(int pin, int val) {
    pin_states[pin] = val;
}

unsigned long millis() {
    return mocked_millis;
}

// Logic from main.cpp
enum ValveState {
    VALVE_IDLE,
    VALVE_OPENING,
    VALVE_CLOSING
};

ValveState currentValveState = VALVE_IDLE;
unsigned long valveTriggerTime = 0;
const unsigned long VALVE_PULSE_DURATION = 30; // ms
const int VALVE_OPEN_PIN = 4;
const int VALVE_CLOSE_PIN = 5;

void triggerValveOpen() {
    if (currentValveState != VALVE_IDLE) return;
    digitalWrite(VALVE_OPEN_PIN, 1); 
    currentValveState = VALVE_OPENING;
    valveTriggerTime = millis();
}

void processStateMachine() {
    if (currentValveState != VALVE_IDLE) {
        if (millis() - valveTriggerTime >= VALVE_PULSE_DURATION) {
            if (currentValveState == VALVE_OPENING) {
                digitalWrite(VALVE_OPEN_PIN, 0);
            } else if (currentValveState == VALVE_CLOSING) {
                digitalWrite(VALVE_CLOSE_PIN, 0);
            }
            currentValveState = VALVE_IDLE;
        }
    }
}

void setUp(void) {
    mocked_millis = 0;
    currentValveState = VALVE_IDLE;
    pin_states[VALVE_OPEN_PIN] = 0;
    pin_states[VALVE_CLOSE_PIN] = 0;
}
void tearDown(void) {}

void test_valve_open_timing(void) {
    triggerValveOpen();
    TEST_ASSERT_EQUAL(VALVE_OPENING, currentValveState);
    TEST_ASSERT_EQUAL(1, pin_states[VALVE_OPEN_PIN]);
    
    // Simulate 10ms passing
    mocked_millis = 10;
    processStateMachine();
    TEST_ASSERT_EQUAL(VALVE_OPENING, currentValveState);
    TEST_ASSERT_EQUAL(1, pin_states[VALVE_OPEN_PIN]);
    
    // Simulate 30ms passing
    mocked_millis = 30;
    processStateMachine();
    TEST_ASSERT_EQUAL(VALVE_IDLE, currentValveState);
    TEST_ASSERT_EQUAL(0, pin_states[VALVE_OPEN_PIN]);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_valve_open_timing);
    UNITY_END();
    return 0;
}
