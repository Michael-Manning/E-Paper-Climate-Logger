#include <cstring>

#include <Arduino.h>
#include <Wire.h>

#include "EEPROM.h"
#include "MAssert.h"
#include "debugLog.h"

namespace flash{

   // poll device for completion of the internal write‑cycle (max 5 ms)
   static bool _ackPoll()
   {
      const uint32_t t_start = millis();
      while (millis() - t_start < 10) {               // double the spec margin
         Wire.beginTransmission(_24LC512_Addr);
         if (Wire.endTransmission() == 0) return true; // ACK ready
      }
      return false; // timed‑out
   }

   bool readByte(uint16_t address, uint8_t &result)
   {
      Wire.beginTransmission(_24LC512_Addr);
      Wire.write(highByte(address));  // MSB first
      Wire.write(lowByte(address));   // LSB
      if (Wire.endTransmission(false) != 0) return false;   // restart
      
      if (Wire.requestFrom((int)_24LC512_Addr, 1) != 1) return false;
      result = Wire.read();
      return true;
   }

   bool readPage(uint16_t startAddress, uint8_t *data, int count)
   {
      if (count <= 0) return false;
      
      Wire.beginTransmission(_24LC512_Addr);
      Wire.write(highByte(startAddress));
      Wire.write(lowByte(startAddress));
      if (Wire.endTransmission(false) != 0) return false;
      
      if (Wire.requestFrom((int)_24LC512_Addr, count) != count) return false;
      for (int i = 0; i < count; ++i) data[i] = Wire.read();
      return true;
   }

   bool writeByte(uint16_t address, uint8_t value)
   {
   Wire.beginTransmission(_24LC512_Addr);
   Wire.write(highByte(address));
    Wire.write(lowByte(address));
    Wire.write(value);
    if (Wire.endTransmission() != 0) return false;
    
    return _ackPoll();
   }
   
   bool writePage(uint16_t startAddress, const uint8_t *value, int count)
   {
      constexpr uint8_t PAGE_SIZE = 128;
      if (count <= 0 || count > PAGE_SIZE) return false;
      
      // ensure page boundary is not crossed
      uint8_t inPage = PAGE_SIZE - (startAddress % PAGE_SIZE);
      if (count > inPage) return false;
      
      Wire.beginTransmission(_24LC512_Addr);
      Wire.write(highByte(startAddress));
      Wire.write(lowByte(startAddress));
      for (int i = 0; i < count; ++i) Wire.write(value[i]);
      if (Wire.endTransmission() != 0) return false;
      
      return _ackPoll();
   }
}
   
using namespace flash;

// Thank you chatGPT for this namespace
namespace{
   constexpr uint8_t UNUSED = 0xFFu;      /* value found in erased EEPROM        */
   #define HALF_RANGE  128u       /* 256/2; use 32768u for 16‑bit index  */

   /* ------------------------------------------------------------------ */
   /*  Scan slotIndexes[] and return:                                     *
   *      - lastSlot  : index of the most‑recently written page          *
   *      - lastIndex : its header index                                 *
   *  Return –1 if no valid page exists (all slots are UNUSED).          */
   /* ------------------------------------------------------------------ */
   static int16_t findLastSlot(const uint8_t* slotIndexes,
      size_t pageCount,
      uint8_t* lastIndex)
   {
      int16_t lastSlot = -1;
      uint8_t idx = 0;

      for (size_t i = 0; i < pageCount; ++i) {
         uint8_t v = slotIndexes[i];
         if (v == UNUSED)
            continue;                       /* empty slot – ignore      */

         if (lastSlot < 0) {                 /* first valid slot seen    */
            lastSlot = (int16_t)i;
            idx = v;
         }
         else {
            uint8_t d = (uint8_t)(v - idx); /* distance forward in mod‑256 */
            if (d != 0 && d < HALF_RANGE) { /* newer than current best  */
               lastSlot = (int16_t)i;
               idx = v;
            }
         }
      }

      if (lastSlot >= 0)
         *lastIndex = idx;

      return lastSlot;
   }


   void selectNextSlot(const uint8_t* slotIndexes,
      size_t pageCount,
      uint8_t* slotToWrite,
      uint8_t* nextIndex)
   {
      uint8_t lastIdx;
      int16_t lastSlot = findLastSlot(slotIndexes, pageCount, &lastIdx);

      if (lastSlot < 0) {            /* EEPROM completely blank           */
         *slotToWrite = 0;
         *nextIndex = 0;          /* first valid index                  */
      }
      else {
         *slotToWrite = (uint8_t)((lastSlot + 1U) % pageCount);

         uint8_t tmp = (uint8_t)(lastIdx + 1U);   /* increment modulo‑256 */
         if (tmp == UNUSED)                       /* 0xFF reserved         */
            tmp = 0;                             /* wrap to 0             */
         *nextIndex = tmp;
      }
   }


   bool wearLevelSelfCheck(uint8_t* slotIndexes, uint16_t pageCount)
   {
      int minIndex = 0;
      bool containsUnused = false;
      bool validWithUnused = true;
      for (int i = 0; i < pageCount; i++)
      {
         if (slotIndexes[i] < slotIndexes[minIndex])
            minIndex = i;
         if (slotIndexes[i] == UNUSED)
            containsUnused = true;
         else
            validWithUnused |= slotIndexes[i] == i;
         if (containsUnused && slotIndexes[i] != UNUSED)
            return false;
      }

      if (containsUnused == true)
         return validWithUnused;

      for (int i = 0; i < pageCount - 1; i++) {
         uint8_t a = slotIndexes[(minIndex + i) % pageCount];
         uint8_t b = slotIndexes[(minIndex + i + 1) % pageCount];
         if (a != b - 1 && b != UNUSED - (pageCount - 1 - a))
            return false;
      }
      return true;
   }
}





bool EEPROM::ValidateHeaderState(){

   bool res = true;
   for (int i = 0; i < headerPages; i++)
   {
      // index if first byte of every page
      uint8_t index = 0;
      res &= readByte(headerStartAddress + (i * pageSize), index);
      headerSlotIndexes[i] = index;
   }

   res &= wearLevelSelfCheck(headerSlotIndexes, headerPages);
   headersRead = true;
   return res;
}


bool EEPROM::ReadSettings(Settings& settings){
   uint8_t settingsData[sizeof(Settings)];
   bool res = readPage(settingsAddress, settingsData, (int)sizeof(settings));
   if(!res)
      return false;
   std::memcpy(&settings, settingsData, sizeof(Settings));
   return true;
}
bool EEPROM::WriteSettings(const Settings& settings){
   const uint8_t* settingsData = reinterpret_cast<const uint8_t*>(&settings);
   return writePage(settingsAddress, settingsData, (int)sizeof(Settings));
}

EEPROM::HeaderResult EEPROM::GetLastStatusHeader(StatusHeader& header){

    MASSERT(headersRead);

    uint8_t lastIndex = 0;
    int16_t lastSlot = findLastSlot(headerSlotIndexes, headerPages, &lastIndex);

    if(lastSlot == -1)
        return HeaderResult::NoHeadersFound;

    uint8_t headerData[sizeof(StatusHeader)];
    bool res = readPage(headerStartAddress + lastSlot * pageSize, headerData, (int)sizeof(StatusHeader));
    if(!res)
        return HeaderResult::ReadError;

    MASSERT(header.dataStart < flash::dataCapacity);
    MASSERT(header.dataCount <= flash::dataCapacity);

    std::memcpy(&header, headerData, sizeof(StatusHeader));
    return HeaderResult::Success;
}


bool EEPROM::StartNextStatusHeader(StatusHeader& header){
   MASSERT(headersRead);

   uint8_t slotToWrite = 0, nextIndex = 0;
   selectNextSlot(headerSlotIndexes, headerPages, &slotToWrite, &nextIndex);
   currentHeaderSlot = slotToWrite;
   header.index = nextIndex;
   
   return UpdateStatusHeader(header);
}

bool EEPROM::UpdateStatusHeader(const StatusHeader& header){
    if(currentHeaderSlot == -1)
        return false;

    logf("writing to slot %d index %d\n", currentHeaderSlot, header.index);
    const uint8_t* headerData = reinterpret_cast<const uint8_t*>(&header);
    return writePage(headerStartAddress + currentHeaderSlot * pageSize, headerData, (int)sizeof(StatusHeader));
}

bool EEPROM::AddClimateSample(StatusHeader& header, const ClimateSample& sample){

    if (header.dataStart >= flash::dataCapacity) 
        header.dataStart %= flash::dataCapacity;

    if (header.dataCount >  flash::dataCapacity) 
        header.dataCount =  flash::dataCapacity;

    const uint16_t writeIndex = uint16_t((header.dataStart + header.dataCount) % flash::dataCapacity);
    const uint32_t addr = flash::dataStartAddress + writeIndex * (uint16_t)sizeof(ClimateSample);

    const uint8_t* p = reinterpret_cast<const uint8_t*>(&sample);
    for (int i = 0; i < sizeof(ClimateSample); ++i) {
        if (!writeByte(uint16_t(addr + i), p[i])) {
            return false; 
        }
    }

    // Advance ring metadata
    if (header.dataCount < flash::dataCapacity) {
        header.dataCount++;
    } 
    else {
        // buffer full: overwrite oldest; move start forward by one
        header.dataStart = uint16_t((header.dataStart + 1) % flash::dataCapacity);
        // dataCount stays at dataCapacity;
    }
    return true;
}

bool EEPROM::FactoryReset(){
   bool res = true;
   for (int i = 0; i < headerPages; i++)
   {
      res &= writeByte(headerStartAddress + (i * pageSize), UNUSED);
      headerSlotIndexes[i] = UNUSED;
   }
   return res;
}

// Thank you ChatGPT for this function
bool EEPROM::GetClimateSamples(const StatusHeader& header,
                               ClimateSample* buffer,
                               uint16_t rangeStart,
                               uint16_t rangeEnd,
                               uint16_t& count)
{
    uint32_t timestamp = micros();

    count = 0;
    if (!buffer) return false;

    constexpr uint16_t PAGE_SIZE = flash::pageSize;           // 128
    const uint16_t cap           = flash::dataCapacity;       // samples
    if (cap == 0) return true;

    // Samples actually present, capped to capacity
    const uint16_t available = (header.dataCount > cap) ? cap : header.dataCount;
    if (available == 0) return true;                          // nothing to read

    // Normalize the requested half-open window [lo, hi) in "ago" units (0=newest)
    uint16_t lo = (rangeStart < rangeEnd) ? rangeStart : rangeEnd;
    uint16_t hi = (rangeStart < rangeEnd) ? rangeEnd   : rangeStart;

    // Clamp to available range [0, available)
    if (lo >= available) return true;                         // window entirely outside
    if (hi >  available) hi = available;
    const uint16_t need = uint16_t(hi - lo);
    if (need == 0) return true;

    const uint32_t baseAddr   = uint32_t(flash::dataStartAddress);
    const uint32_t sampleSize = uint32_t(sizeof(ClimateSample));

    // Defensive: normalize start index to [0, cap)
    const uint16_t startIndex = (header.dataStart < cap)
                              ? header.dataStart
                              : uint16_t(header.dataStart % cap);

    // Iterate oldest->newest within the window: d = hi-1, hi-2, ..., lo
    for (uint32_t d = uint32_t(hi) - 1; ; )
    {
        // Convert "ago" (d) to physical ring index
        // newest index = (startIndex + available - 1) % cap
        // desired index = newest - d  (mod cap)
        const uint16_t ringIndex =
            uint16_t((uint32_t(startIndex) + uint32_t(available - 1) - d) % cap);

        // Absolute EEPROM address of the start of this sample
        uint32_t addr = baseAddr + uint32_t(ringIndex) * sampleSize;

        // Destination in caller's buffer
        uint8_t* dst  = reinterpret_cast<uint8_t*>(buffer) + uint32_t(count) * sampleSize;

        // Read one sample, possibly split across page boundaries
        uint32_t remaining = sampleSize;
        while (remaining > 0)
        {
            const uint16_t inPage = PAGE_SIZE - uint16_t(addr % PAGE_SIZE);
            const uint16_t chunk  = (remaining < inPage) ? uint16_t(remaining) : inPage;

            if (!flash::readPage(uint16_t(addr), dst, int(chunk)))
                return false;                 // partial count already set

            addr      += chunk;
            dst       += chunk;
            remaining -= chunk;
        }

        ++count;

        if (d == lo) break;                  // finished the window
        --d;
    }

    logf("read %d samples in %dus\n", count, (int)(micros() - timestamp));

    return true;
}