#pragma once

#include <stdint.h>
#include "drivers/SHT45.h"

constexpr int _24LC512_Addr = 0x50;

namespace flash{   
   
   constexpr int capacity = 0xFFFF;
   constexpr int pageSize = 128;
   constexpr int maxPage = capacity / pageSize;
   constexpr uint16_t settingsAddress = 0x0000;
   constexpr int settingsPages = 1; // assumed to be 1 in multiple places
   constexpr uint16_t headerStartAddress = settingsAddress + settingsPages * pageSize;
   constexpr int headerPages = 20; // decreases wear, but also decreases data capacity
   constexpr uint16_t dataStartAddress = headerStartAddress + headerPages * pageSize;
   constexpr uint16_t dataCapacity = (capacity - dataStartAddress) / sizeof(ClimateSample);

   bool readByte(uint16_t address, uint8_t& result); // perform a random read
   bool readPage(uint16_t startAddress, uint8_t* data, int count); // perform a Sequential Read
   bool writeByte(uint16_t address, uint8_t value); // write a single byte
   bool writePage(uint16_t startAddress, const uint8_t* value, int count); // perform a page write
}

enum class ShutDownReason : uint8_t{
    PowerOff  = 0, // completely powered off
    Hibernate = 1, // long term display shutdown
    FirmwareUpdate = 2, 
    Unplanned = 3,  // not yet shut down. Reading this state back indicates invalid power down
    LowBattery = 4 
};

// unused
enum class DisplayType : uint8_t {
    Temp_Humi_Time = 0,
    Temp_Graph30m  = 1,
    Temp_Graph1h   = 2,
    Temp_Graph3h   = 3
};

enum class DisplayRefreshState : uint8_t{
    Fresh,
    FullRefresh,
    PartialFullBlank,
    Partial
};

struct StatusHeader{
   uint8_t index                   : 8 = 0;
   ShutDownReason shutDownReason   : 3;
   DisplayType selectedDisplayType : 2; // unused
   bool displayCold                : 3;
   uint8_t partialRefreshCount     : 8 = 0;
   DisplayRefreshState refreshState: 8;
   uint16_t dataStart              : 16;
   uint16_t dataCount              : 16;
   uint32_t checksum               : 32;
};
static_assert(sizeof(StatusHeader) < flash::pageSize);

static inline StatusHeader FreshHeader(){
    return StatusHeader{
        .shutDownReason = ShutDownReason::Unplanned,
        .selectedDisplayType = DisplayType::Temp_Humi_Time,
        .displayCold = false,
        .partialRefreshCount = 0,   
        .refreshState = DisplayRefreshState::Fresh,
        .dataStart = 0,
        .dataCount = 0,
        .checksum = 0
    };
}

enum class TimeFormat : uint8_t{
   _12Hour = 0,
   _24Hour = 1
};
enum class GraphRange : uint8_t{
    _10Minutes = 0,
    _20Minutes = 1,
    _30Minutes = 2,
    _60Minutes = 3,
};

struct Settings{
    TemperatureFormat tempFormat : 1;
    TimeFormat timeFormat        : 1;
    GraphRange graphRange        : 4;
};
static_assert(sizeof(Settings) < flash::pageSize);

static inline Settings DefaultSettings(){
    return Settings{
        .tempFormat = TemperatureFormat::Celcius,
        .timeFormat = TimeFormat::_12Hour,
        .graphRange = GraphRange::_60Minutes
    };
};

class EEPROM{
public:

   enum class HeaderResult{
      Success,
      ReadError,
      NoHeadersFound
   };

   bool ReadSettings(Settings& settings);
   bool WriteSettings(const Settings& settings);
   
   HeaderResult GetLastStatusHeader(StatusHeader& header);

   bool StartNextStatusHeader(StatusHeader& header);
   bool UpdateStatusHeader(const StatusHeader& header);

   bool ValidateHeaderState();

   // add sample to ring buffer which occupies entire eeprom after header buffer
   bool AddClimateSample(StatusHeader& header, const ClimateSample& sample);

   // ranges are in "samples ago" from the most recent sample (which is effectively minutes ago)
   bool GetClimateSamples(const StatusHeader& header, ClimateSample* buffer, uint16_t rangeStart, uint16_t rangeEnd, uint16_t& count);

   // sets all status header indexes to unused status
   // resets settings to their default values.
   // resets current time ?
   bool FactoryReset();

private:
   bool headersRead = false;
   int currentHeaderSlot = -1;
   uint8_t headerSlotIndexes[flash::headerPages];
};