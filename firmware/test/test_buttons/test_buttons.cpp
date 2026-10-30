// Buttons test (physical, interactive): reads raw GPIO levels directly, bypassing all
// firmware debounce/multipress logic. Requires the operator to press each button in turn.
#include <Arduino.h>
#include <unity.h>

#include "../test_common.h"
#include "pinout.h"

void setUp(void) {}
void tearDown(void) {}

void test_menu_button(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestWaitForPinLow(pins::menuBtn, 10000, "Menu button (SW1)"),
                              "no press detected within 10s");
}

void test_power_button(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestWaitForPinLow(pins::powerBtn, 10000, "Power button (SW2)"),
                              "no press detected within 10s");
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_menu_button);
    RUN_TEST(test_power_button);
    UNITY_END();
}

void loop() {}
