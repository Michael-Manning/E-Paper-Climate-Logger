#pragma once

#include <Wire.h>
#include <Stream.h>

struct DateTime 
{      
   uint8_t Second;
   uint8_t Minute;
   uint8_t Hour;
   uint8_t Dow;
   uint8_t Day;
   uint8_t Month;
   uint8_t Year;  
};

inline static void GetTimeString12(char* buffer, const DateTime& time){
    int hour12 = time.Hour % 12;
    if (hour12 == 0) 
        hour12 = 12; // 0 or 12 becomes 12 in 12-hour time
    
    sprintf(buffer, "%d:%02d", hour12, (int)time.Minute);
};

inline static void GetTimeString24(char* buffer, const DateTime& time){
    sprintf(buffer, "%d:%02d", (int)time.Hour, (int)time.Minute);
};

inline static void GetTimeStringFull12(char* buffer, const DateTime& time){
    int hour12 = time.Hour % 12;
    if (hour12 == 0) 
        hour12 = 12; // 0 or 12 becomes 12 in 12-hour time
    sprintf(buffer, "%02d:%02d:%02d", hour12, (int)time.Minute, (int)time.Second);
};

inline static void GetTimeStringFull24(char* buffer, const DateTime& time){
    sprintf(buffer, "%02d:%02d:%02d", (int)time.Hour, (int)time.Minute, (int)time.Second);
};

class DS3231
{
  public:
    


   union Control 
   {
      struct 
      {
         uint8_t alarm1InterruptEnable : 1;
         uint8_t alarm2InterruptEnable : 1;
         uint8_t interruptControl      : 1;
         uint8_t _reserved             : 2;
         uint8_t convertTemperature    : 1;
         uint8_t squareWaveEnable      : 1;
         uint8_t disableOscillator     : 1;
      } bits;
      uint8_t raw = 0;
   };

   union Status
   {
      struct{
         uint8_t alarm1Flag     : 1;
         uint8_t alarm2Flag     : 1;
         uint8_t busy           : 1;
         uint8_t enabled32KHZ   : 1;
         uint8_t _reserved      : 2;
         uint8_t OscillatorStop : 1;
      } bits;
      uint8_t raw = 0;
   };
   
   
   DateTime Read();
   uint8_t Write(const DateTime&);
   
   bool SetControl(Control control);
   bool GetStatus(Status& status);
   bool SetStatus(Status status);
   bool SetAlarm1(uint8_t secondsMatch);
   bool CheckAlarm();
   bool ClearAlarm1();

   private: 
   
     static const uint8_t      RTC_ADDRESS  = 0x68;    
       
     static uint8_t rtc_i2c_seek(const uint8_t Address);
     static uint8_t rtc_i2c_write_byte(const uint8_t Address, const uint8_t Byte);    
     static uint8_t rtc_i2c_read_byte(const uint8_t Address,  uint8_t &Byte);    
    
};
