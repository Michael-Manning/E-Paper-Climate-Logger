// RTC (DS3231) test: presence, write/read-back, oscillator running, and the alarm INT pin.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "../test_common.h"
#include "drivers/DS3231.h"

namespace {
DS3231 _clock;
}

void setUp(void) {}
void tearDown(void) {}

void test_rtc_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(0x68), "RTC did not ACK on the I2C bus");
}

void test_write_read_back(void) {
    DateTime before = _clock.Read();

    DateTime testTime;
    testTime.Second = 30;
    testTime.Minute = 15;
    testTime.Hour = 10;
    testTime.Dow = 3;
    testTime.Day = 15;
    testTime.Month = 6;
    testTime.Year = 24;
    _clock.Write(testTime);
    delay(50);
    DateTime readBack = _clock.Read();

    // restore the original time; a few seconds of drift from running this test is expected --
    // re-set the clock from the main firmware's Set Time menu afterward if precision matters
    _clock.Write(before);

    TEST_ASSERT_EQUAL_UINT8(testTime.Hour, readBack.Hour);
    TEST_ASSERT_EQUAL_UINT8(testTime.Minute, readBack.Minute);
    TEST_ASSERT_EQUAL_UINT8(testTime.Day, readBack.Day);
    TEST_ASSERT_EQUAL_UINT8(testTime.Month, readBack.Month);
    TEST_ASSERT_EQUAL_UINT8(testTime.Year, readBack.Year);
    TEST_ASSERT_INT_WITHIN(1, testTime.Second, readBack.Second);
}

void test_oscillator_ticks(void) {
    DateTime t1 = _clock.Read();
    delay(2000);
    DateTime t2 = _clock.Read();

    int secDelta = (int)t2.Second - (int)t1.Second;
    if (secDelta < 0) secDelta += 60;

    TEST_ASSERT_INT_WITHIN_MESSAGE(1, 2, secDelta, "RTC did not advance ~2s in 2s -- check the 32.768kHz crystal/oscillator");
}

void test_alarm_interrupt_pin(void) {
    uint8_t alarmSec = (_clock.Read().Second + 2) % 60;

    DS3231::Control c;
    c.bits.interruptControl = 1;
    c.bits.alarm1InterruptEnable = 1;
    _clock.SetControl(c);
    _clock.ClearAlarm1();
    _clock.SetAlarm1(alarmSec);

    TEST_ASSERT_TRUE_MESSAGE(plt::getPinLevel(pins::clockAlarm) != 0,
                              "clockAlarm pin is not idle-HIGH before the alarm fires");

    uint32_t t0 = millis();
    bool fired = false;
    while (millis() - t0 < 4000) {
        if (_clock.CheckAlarm()) {
            fired = true;
            break;
        }
        delay(20);
    }
    _clock.ClearAlarm1();

    TEST_ASSERT_TRUE_MESSAGE(fired, "clockAlarm pin never went low -- check RTC INT/SQW wiring");
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_rtc_present);
    RUN_TEST(test_write_read_back);
    RUN_TEST(test_oscillator_ticks);
    RUN_TEST(test_alarm_interrupt_pin);
    UNITY_END();
}

void loop() {}
