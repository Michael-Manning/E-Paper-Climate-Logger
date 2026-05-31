#include <stdint.h>

#include <Arduino.h>
#include <Wire.h>

#include "pinout.h"
#include "SHT45.h"

// Calculate CRC-8 according to SHT4x (Sensirion) algorithm
uint8_t sht45_crc8(const uint8_t *buf, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; ++i) {
    crc ^= buf[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

bool getClimate(ClimateSample& sample){

    uint8_t register_address = 0xFD; // high precision reading
    uint8_t data[6];

    // Request to set the register pointer
    Wire.beginTransmission(SHT45_Addr);
    Wire.write(register_address);
    if (Wire.endTransmission(false) != 0) {  // false to send a repeated start
        Serial.println("Failed to write register address");
        return false;
    }

    Wire.beginTransmission(SHT45_Addr);
    Wire.write(0xFD);          
    Wire.endTransmission();   

    // wait for aquisition
    delay(11);

    // Request 6 bytes from the device
    if (Wire.requestFrom(SHT45_Addr, (uint8_t)6) != 6) {
        Serial.println("Failed to read 6 bytes");
        return false;
    }

    for (int i = 0; i < 6; i++) {
        if (Wire.available()) {
            data[i] = Wire.read();
        } else {
            Serial.println("Insufficient data available");
            return false;
        }
    }

    // // Output read bytes
    // Serial.print("Received: ");
    // for (int i = 0; i < 6; i++) {
    //     Serial.printf("%X ", data[i]);
    // }
    // Serial.println();




    // Validate temperature CRC
    if (sht45_crc8(data + 0, 2) != data[2]) {
        Serial.printf("SHT45 T checksum failed\n");
        return false;
    }
    // Validate humidity CRC
    if (sht45_crc8(data + 3, 2) != data[5]) {
         Serial.printf("SHT45 H checksum failed\n");
        return false;
    }

    // Combine MSB + LSB for temperature and humidity
    uint16_t rawT = uint16_t(data[0] << 8) | data[1];
    uint16_t rawH = uint16_t(data[3] << 8) | data[4];

    // Convert raw values to physical units
    sample.temperature_raw = static_cast<uint16_t>(std::roundf((-45.0f + 175.0f * (float(rawT) / 65535.0f)) *100.0f));
    sample.humidity_raw = static_cast<uint16_t>(std::roundf((-6.0f + 125.0f * (float(rawH) / 65535.0f)) *100.0f)); 

    return true;
}