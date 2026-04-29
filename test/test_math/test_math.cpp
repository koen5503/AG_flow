#include <unity.h>

#define CALIBRATION_FACTOR 5.5

float calculateVolume(unsigned long pulses) {
    return pulses / ((float)CALIBRATION_FACTOR * 60.0);
}

void test_volume_calculation() {
    // 330 pulses / (5.5 * 60) = 1.0 liter
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, calculateVolume(330)); 
    // 165 pulses / (5.5 * 60) = 0.5 liter
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, calculateVolume(165)); 
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, calculateVolume(0)); 
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_volume_calculation);
    return UNITY_END();
}
