#pragma once

#include <stdint.h>

enum class PinMode{
   Input = 0,
   Input_Pulldown = 1,
   Input_Pullup = 2,
   Output = 3
};

namespace plt{

   void setPinMode(int pin, PinMode mode);

   void setPinLevel(int pin, uint32_t level);
   
   int getPinLevel(int pin);
}