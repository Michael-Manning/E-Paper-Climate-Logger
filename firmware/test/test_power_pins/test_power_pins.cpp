// USB detect / charge status pins test.
//
// This board's native USB connector is both the serial/programming channel `pio test` reads
// results over AND what USBDetect senses -- unplugging it to force a transition also kills
// the connection needed to report the result, so USBDetect can only ever read CONNECTED here.
// There's no way to observe a real transition through this harness: if it were disconnected,
// the test couldn't be running at all. So this only reports current pin state for manual
// review; verifying an actual unplug/replug transition needs a separate probe (multimeter, or
// watching the real app's ChargingScreen behavior) outside of pio test.
#include <Arduino.h>
#include <unity.h>

#include "../test_common.h"
#include "power.h"

void setUp(void) {}
void tearDown(void) {}

void test_report_pin_levels(void) {
    char msg[160];
    snprintf(msg, sizeof(msg),
             "USBConnected=%s chargeStatus=%s fuelGaugeGPIO=%d powerKill=%d "
             "(do not drive powerKill low -- it cuts power immediately)",
             power::USBConnected() ? "CONNECTED" : "DISCONNECTED",
             power::ChargeStatus() ? "CHARGING" : "NOT CHARGING", plt::getPinLevel(pins::fuelGaugeGPIO),
             plt::getPinLevel(pins::powerKill));
    TEST_MESSAGE(msg);
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_report_pin_levels);
    UNITY_END();
}

void loop() {}
