// Confirms every I2C peripheral on the board ACKs at its expected address.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "../test_common.h"
#include "drivers/BQ27441.h"
#include "drivers/EEPROM.h"
#include "drivers/SHT45.h"

void setUp(void) {}
void tearDown(void) {}

void test_eeprom_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(_24LC512_Addr), "EEPROM (24LC512) did not ACK at 0x50");
}

void test_rtc_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(0x68), "RTC (DS3231) did not ACK at 0x68");
}

void test_climate_sensor_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(SHT45_Addr), "Climate sensor (SHT45) did not ACK at 0x44");
}

void test_fuel_gauge_present(void) {
    if (!hwtestI2CPresent(BQ72441_I2C_ADDRESS))
        TEST_IGNORE_MESSAGE("Fuel gauge (BQ27441) did not ACK at 0x55 -- expected if no battery is connected");
}

void setup() {
    delay(2000);  // let the serial monitor attach
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_eeprom_present);
    RUN_TEST(test_rtc_present);
    RUN_TEST(test_climate_sensor_present);
    RUN_TEST(test_fuel_gauge_present);
    UNITY_END();
}

void loop() {}
