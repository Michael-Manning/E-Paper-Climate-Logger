#pragma once

#include <Arduino.h>

// affects:
// - delay on boot when plugged
// - messages printed over serial
// - disables the charging screen when plugged in

// #define EnableLogger

extern bool debugLogUSBConnected;

#ifdef EnableLogger
constexpr bool LoggerEnabled = true;
#else
constexpr bool LoggerEnabled = false;
#endif

inline void logf(const char* fmt, ...) {
#ifdef EnableLogger
    if(debugLogUSBConnected){
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
#else
    (void)fmt;
#endif
}

inline void logl(const char* str) {
#ifdef EnableLogger
    if(debugLogUSBConnected){
        Serial.println(str);
    }
#else
    (void)str;
#endif
}