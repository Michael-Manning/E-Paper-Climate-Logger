#pragma once

#include <stdint.h>

namespace Constants
{  
    //shows a small "s" on the standard display to indicate deep sleep mode. Mainly for debugging purposes
    constexpr bool sleepIndicator = false;

    // if the temperature is less than this, show a low temperature warning instead of trying to drive the display
    // Warning: setting this too high, even for testing purposes, will prevent the device from being programmed until the temperature is reached.
    constexpr float lowTemperatureDisplayThreshold_C = 3.0f; // display is rated for 0 degrees C, but it starts to struggle at low temperatures.

    // when the battery is considered "dead"
    constexpr float lowBatteryThreshold_V = 3.4f;

    // how many partial refreshes can happen before forcing a full refresh (excluding menu interaction)
    constexpr int maxPartialFreshres = 5;
} 