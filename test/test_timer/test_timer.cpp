#include <unity.h>

unsigned long mock_millis_value = 0;
unsigned long millis() {
    return mock_millis_value;
}

enum ValveState {
    VALVE_IDLE,
    VALVE_OPENING,
    VALVE_CLOSING
};

ValveState currentValveState = VALVE_IDLE;
unsigned long valveTriggerTime = 0;
const unsigned long VALVE_PULSE_DURATION = 30;

void triggerValveOpen() {
    currentValveState = VALVE_OPENING;
    valveTriggerTime = millis();
}

void updateValveStateMachine() {
    if (currentValveState != VALVE_IDLE) {
        if (millis() - valveTriggerTime >= VALVE_PULSE_DURATION) {
            currentValveState = VALVE_IDLE;
        }
    }
}

void test_valve_state_machine() {
    mock_millis_value = 100;
    currentValveState = VALVE_IDLE;
    
    triggerValveOpen();
    TEST_ASSERT_EQUAL(VALVE_OPENING, currentValveState);
    TEST_ASSERT_EQUAL(100, valveTriggerTime);
    
    mock_millis_value = 129;
    updateValveStateMachine();
    TEST_ASSERT_EQUAL(VALVE_OPENING, currentValveState);
    
    mock_millis_value = 130;
    updateValveStateMachine();
    TEST_ASSERT_EQUAL(VALVE_IDLE, currentValveState);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_valve_state_machine);
    return UNITY_END();
}
