// PlatformIO's auto-generated Unity config calls Serial.end() from unityOutputComplete() once
// a test suite finishes. On this ESP32-S3 board, Serial is the native USB-Serial-JTAG
// peripheral (ARDUINO_USB_MODE=1), not a separate UART-to-USB bridge chip -- Serial.end()
// (HWCDC::end()) fully disables its interrupts and deinitializes the peripheral, which drops
// the board off the host's USB bus entirely. Recovering it then requires holding BOOT and
// reflashing, once per test suite. Skipping that call keeps the connection alive so the next
// suite can run without any manual intervention.
#include <Arduino.h>

#include "unity_config.h"

void unityOutputStart(unsigned long baudrate) {
    Serial.begin(baudrate);
}
void unityOutputChar(unsigned int c) {
    Serial.write((uint8_t)c);
}
void unityOutputFlush(void) {
    Serial.flush();
}
void unityOutputComplete(void) {}
