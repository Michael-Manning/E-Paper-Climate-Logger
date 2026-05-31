#include <Arduino.h>
#include <Wire.h>

#include "platform.h"
#include "pinout.h"
#include "DS3231.h"


namespace
{

// Page 11 of Datasheet shows that the LSB 4 bits of the data types are always the last BCD digit
//  the upper 3 bits are sometimes the upper digit, sometimes a mix of that and some other data
//  so be sure to mask your input for bcd2bin
   uint8_t bcd2bin (uint8_t val) { return ((val >> 4) * 10) + (val & 0x0F); }
   uint8_t bin2bcd (uint8_t val) { return (val / 10) << 4 | (val % 10);     }
}


uint8_t DS3231::rtc_i2c_seek(const uint8_t Address)
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(Address);
  return Wire.endTransmission();
}

uint8_t DS3231::rtc_i2c_write_byte(const uint8_t Address, const uint8_t Data)
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(Address);
  Wire.write(Data);
  return Wire.endTransmission();
}

uint8_t DS3231::rtc_i2c_read_byte(const uint8_t Address, uint8_t &Data)
{
  rtc_i2c_seek(Address);

  Wire.requestFrom(RTC_ADDRESS,(uint8_t) 1);
  Data = Wire.read();
  return 1;

}


DateTime DS3231::Read()
{
  DateTime currentDate;
  uint8_t  x; 

  // Set the register address by doing a write of just the address
  rtc_i2c_seek(0x00);

  // Read in the 7 bytes which store the
  //  Seconds, Minutes, Hours, Day-Of-Week, Day, Month, Year
  if(Wire.requestFrom(RTC_ADDRESS,(uint8_t) 7) == 7)
  {
    currentDate.Second = bcd2bin(Wire.read());
    currentDate.Minute = bcd2bin(Wire.read());
    
    // 6th Bit of hour indicates 12/24 Hour mode, we will always use 24 hour mode, because we is smart
    x = Wire.read();    
    if(x & _BV(6))
    {
      currentDate.Hour = bcd2bin(x & 0B11111) + (x & _BV(5) ? 0 : 12);
    }
    else
    {
      currentDate.Hour = bcd2bin(x & 0B111111);
    }
    
    currentDate.Dow = bcd2bin(Wire.read());
    currentDate.Day = bcd2bin(Wire.read());
    
    x = Wire.read();
    // bit 7 of month indicates if the year is going to be 100+Year or just Year
    if(x&_BV(7))
    {
      currentDate.Year = 100;
    }
    else
    {
      currentDate.Year = 0;
    }
    currentDate.Month = bcd2bin(x & 0B01111111);
    currentDate.Year += bcd2bin(Wire.read());
  }  
  return currentDate;
}

uint8_t DS3231::Write(const DateTime &currentDate)
{
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00); // Start address of data
  Wire.write(bin2bcd(currentDate.Second));
  Wire.write(bin2bcd(currentDate.Minute));
  Wire.write(bin2bcd(currentDate.Hour));
  Wire.write(bin2bcd(currentDate.Dow?currentDate.Dow:1)); // People might not bother with Dow, make sure it's valid, in case.
  Wire.write(bin2bcd(currentDate.Day));
  Wire.write(bin2bcd(currentDate.Month));
  Wire.write(bin2bcd(currentDate.Year));
  return Wire.endTransmission() ? 0 : 1; // endTransmission returns a code in the response, to make it "Simple" we will return 0 for any fail, and 1 for OK
}

bool DS3231::SetControl(Control control){
   rtc_i2c_write_byte(0x0E, control.raw);
   return true;
}
bool DS3231::GetStatus(Status& status){
   rtc_i2c_read_byte(0x0F, status.raw);
   return true;
}
bool DS3231::SetStatus(Status status){
   rtc_i2c_write_byte(0x0F, status.raw);
   return true;
}

bool DS3231::SetAlarm1(uint8_t secondsMatch)
{
   Wire.beginTransmission(RTC_ADDRESS);
   Wire.write(0x07); 
   Wire.write(bin2bcd(secondsMatch & 0b01111111));
   Wire.write(0b10000000); 
   Wire.write(0b10000000); 
   Wire.write(0b10000000); 
   Wire.endTransmission();
   return true;
}

bool DS3231::CheckAlarm(){
   return plt::getPinLevel(pins::clockAlarm) == false;
}

bool DS3231::ClearAlarm1(){
   Status s;
   GetStatus(s);
   s.bits.alarm1Flag = 0;
   SetStatus(s);
   return true;
}
