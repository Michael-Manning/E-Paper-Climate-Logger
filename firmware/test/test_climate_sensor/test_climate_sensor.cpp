// Climate sensor (SHT45) test: presence and a CRC-validated, plausibility-checked reading.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "../test_common.h"
#include "drivers/SHT45.h"

void setUp(void) {}
void tearDown(void) {}

void test_sensor_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(SHT45_Addr), "SHT45 did not ACK on the I2C bus");
}

void test_plausible_reading(void) {
    ClimateSample sample;
    bool ok = false;
    for (int i = 0; i < 5 && !ok; i++) ok = getClimate(sample);
    TEST_ASSERT_TRUE_MESSAGE(ok, "could not read a valid measurement (CRC or I2C error)");

    float tempC = sample.Temperature_c();
    float rh = sample.Humidity_r();

    char msg[64];
    snprintf(msg, sizeof(msg), "T=%.2fC RH=%.2f%% (cup your hand around the sensor to see these move)", tempC, rh);
    TEST_MESSAGE(msg);

    TEST_ASSERT_TRUE_MESSAGE(tempC > -20.0f && tempC < 60.0f, "temperature out of plausible range");
    TEST_ASSERT_TRUE_MESSAGE(rh >= 0.0f && rh <= 100.0f, "humidity out of plausible range");
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_sensor_present);
    RUN_TEST(test_plausible_reading);
    UNITY_END();
}

void loop() {}
