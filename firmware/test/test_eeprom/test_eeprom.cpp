// EEPROM (24LC512) read/write test. Uses the raw flash:: chip-level functions rather than
// the EEPROM class, so this exercises the physical chip independently of the ring-buffer
// bookkeeping logic in EEPROM.cpp.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "../test_common.h"
#include "drivers/EEPROM.h"

void setUp(void) {}
void tearDown(void) {}

// Last full page-aligned page of the chip's address space -- avoids the settings/header
// pages the real application uses. Contents are backed up and restored around the test.
constexpr uint16_t testAddr = (uint16_t)((flash::maxPage - 1) * flash::pageSize);

void test_eeprom_present(void) {
    TEST_ASSERT_TRUE_MESSAGE(hwtestI2CPresent(_24LC512_Addr), "EEPROM did not ACK on the I2C bus");
}

void test_page_write_read_back(void) {
    uint8_t original[flash::pageSize];
    uint8_t pattern[flash::pageSize];
    uint8_t readback[flash::pageSize];

    for (int i = 0; i < flash::pageSize; i++)
        pattern[i] = (uint8_t)((i * 37) ^ 0xA5);  // avoids false positives from an all-0x00/0xFF chip

    TEST_ASSERT_TRUE_MESSAGE(flash::readPage(testAddr, original, flash::pageSize), "could not read page for backup");
    TEST_ASSERT_TRUE_MESSAGE(flash::writePage(testAddr, pattern, flash::pageSize), "page write failed");

    bool readOk = flash::readPage(testAddr, readback, flash::pageSize);
    flash::writePage(testAddr, original, flash::pageSize);  // restore regardless of outcome

    TEST_ASSERT_TRUE_MESSAGE(readOk, "page read-back failed");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(pattern, readback, flash::pageSize, "written and read-back page contents differ");
}

void test_single_byte_read_write(void) {
    uint8_t origByte;
    TEST_ASSERT_TRUE_MESSAGE(flash::readByte(testAddr, origByte), "could not read byte for backup");

    uint8_t testByte = (uint8_t)(~origByte);
    bool wOk = flash::writeByte(testAddr, testByte);
    uint8_t rbByte = 0;
    bool rOk = wOk && flash::readByte(testAddr, rbByte);

    flash::writeByte(testAddr, origByte);  // restore

    TEST_ASSERT_TRUE_MESSAGE(wOk, "single-byte write failed");
    TEST_ASSERT_TRUE_MESSAGE(rOk, "single-byte read failed");
    TEST_ASSERT_EQUAL_UINT8(testByte, rbByte);
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_eeprom_present);
    RUN_TEST(test_page_write_read_back);
    RUN_TEST(test_single_byte_read_write);
    UNITY_END();
}

void loop() {}
