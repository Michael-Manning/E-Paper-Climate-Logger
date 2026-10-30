// Fuel gauge (BQ27441) test. The gauge is powered from the battery, not the board's 3V3 rail,
// so it is expected to be absent (tests IGNORE, not FAIL) whenever no battery is connected.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "../test_common.h"
#include "drivers/BQ27441.h"

void setUp(void) {}
void tearDown(void) {}

void test_fuel_gauge_present(void) {
    if (!hwtestI2CPresent(BQ72441_I2C_ADDRESS))
        TEST_IGNORE_MESSAGE("BQ27441 did not ACK -- is a battery connected? The gauge is battery-powered.");
}

void test_fuel_gauge_reads_plausible_voltage(void) {
    if (!hwtestI2CPresent(BQ72441_I2C_ADDRESS))
        TEST_IGNORE_MESSAGE("skipped -- fuel gauge not present (see test_fuel_gauge_present)");

    TEST_ASSERT_TRUE_MESSAGE(lipo.begin(), "lipo.begin() failed to establish communication");

    uint16_t devType = lipo.deviceType();
    uint16_t voltage = lipo.voltage();

    char msg[64];
    snprintf(msg, sizeof(msg), "deviceType=0x%04X (expect 0x0421) voltage=%umV", devType, voltage);
    TEST_MESSAGE(msg);

    TEST_ASSERT_TRUE_MESSAGE(voltage > 2000 && voltage < 4500, "voltage out of plausible range");
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_fuel_gauge_present);
    RUN_TEST(test_fuel_gauge_reads_plausible_voltage);
    UNITY_END();
}

void loop() {}
