#pragma once

#include <stdint.h>
#include <Arduino.h>

// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no xorout)
static uint16_t crc16_ccitt_false(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; ++b) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

constexpr uint8_t TimeMessageStartByte = 0x5C;

// Non-blocking parser for frames: [0x5C][HH][MM][SS][CRC_hi][CRC_lo]
bool readTimeMessage(Stream& s, uint8_t& hour, uint8_t& minute, uint8_t& second) {
  // Seek the start byte
  while (s.available()) {
    int c = s.peek();
    if (c < 0) return false; // no data
    if ((uint8_t)c == TimeMessageStartByte) {
      break;
    }
    s.read(); // discard until start marker
  }

  // Need full frame: 6 bytes
  if (s.available() < 6) {
    return false; // wait for more
  }

  uint8_t frame[6];
  size_t got = s.readBytes(frame, sizeof(frame));
  if (got != sizeof(frame)) {
    return false;
  }

  // Validate start byte
  if (frame[0] != TimeMessageStartByte) {
    return false;
  }

  // Validate CRC over bytes [0..3]
  uint16_t recvCrc = (uint16_t(frame[4]) << 8) | frame[5];
  uint16_t calcCrc = crc16_ccitt_false(frame, 4);
  if (recvCrc != calcCrc) {
    // CRC failed; try to resync by discarding one byte next time.
    return false;
  }

  // Extract time
  hour   = frame[1];
  minute = frame[2];
  second = frame[3];

  // Optional: range checks (reject out-of-range values)
  if (hour > 23 || minute > 59 || second > 59) {
    return false;
  }

  return true;
}