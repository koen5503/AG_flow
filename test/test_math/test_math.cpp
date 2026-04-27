#include <unity.h>

const float CALIBRATION_FACTOR = 4.5;

float calculateVolume(unsigned long pulses) {
    return pulses / CALIBRATION_FACTOR;
}

void setUp(void) {}
void tearDown(void) {}

void test_volume_calculation(void) {
    TEST_ASSERT_EQUAL_FLOAT(10.0, calculateVolume(45));
    TEST_ASSERT_EQUAL_FLOAT(0.0, calculateVolume(0));
    TEST_ASSERT_EQUAL_FLOAT(2.0, calculateVolume(9));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_volume_calculation);
    UNITY_END();
    return 0;
}
