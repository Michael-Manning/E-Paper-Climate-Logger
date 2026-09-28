#pragma once

// Shared helpers for the PCB hardware bring-up test suites. Included by each test_*.cpp --
// everything here is `inline` since it's compiled directly into whichever single suite
// includes it (each suite is its own firmware binary).

#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "MAssert.h"
#include "pinout.h"
#include "platform.h"
#include "power.h"

// main.cpp normally provides this (invoked by MASSERT on failed asserts in the drivers).
// It's excluded from test builds, so each test binary needs its own definition.
//
// __attribute__((used)) forces the compiler to emit this as a linkable symbol even in test
// suites that never call on_error() by name themselves (e.g. test_display.cpp, which only
// calls into Display methods that invoke MASSERT internally) -- without it, GCC has no way
// to know another translation unit needs a callable copy and silently drops it.
__attribute__((used)) inline void on_error(const char* cond, const char* file, int line) {
    Serial.printf("ASSERT FAILED: (%s) -- %s line: %d\n", cond, file, line);
}

// Mirrors main.cpp's configurePins() + LatchOnOffController() so peripherals see the same
// pin configuration under test as they do in the real application.
inline void hwtestConfigurePins() {
    using namespace plt;
    setPinMode(pins::menuBtn, PinMode::Input_Pullup);
    setPinMode(pins::powerBtn, PinMode::Input);
    setPinMode(pins::powerKill, PinMode::Input);
    setPinMode(pins::USBDetect, PinMode::Input);
    setPinMode(pins::fuelGaugeGPIO, PinMode::Input);
    setPinMode(pins::chargeStatus, PinMode::Input_Pullup);
    setPinMode(pins::clockAlarm, PinMode::Input);

    power::LatchOnOffController();

    Wire.begin(pins::SDA, pins::SCL, 400000);
}

inline bool hwtestI2CPresent(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

// Polls a raw, active-low GPIO (button/switch) and reports whether it went low within the
// timeout. Bypasses all firmware debounce/multipress logic -- this checks the electrical
// path only, driven by an actual physical press from whoever is running the test.
inline bool hwtestWaitForPinLow(int pin, uint32_t timeoutMS, const char* label) {
    char msg[96];
    snprintf(msg, sizeof(msg), "Press and release the %s...", label);
    TEST_MESSAGE(msg);

    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMS) {
        if (plt::getPinLevel(pin) == 0) {
            while (plt::getPinLevel(pin) == 0) delay(10);  // wait for release
            return true;
        }
        delay(10);
    }
    return false;
}
