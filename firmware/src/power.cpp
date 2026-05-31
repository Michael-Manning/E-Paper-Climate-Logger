
#include <Arduino.h>

#include "esp_sleep.h"
#include "driver/rtc_io.h"
// #include "hal/gpio_types.h"

#include "platform.h"
#include "pinout.h"
#include "drivers/BQ27441.h"
#include "drivers/BQ27441_Definitions.h"
#include "power.h"

namespace power{

    bool USBConnected(){
        return plt::getPinLevel(pins::USBDetect);
    }

    bool GetPowerStatus(PowerStatus& status){

        int c = lipo.current(AVG);

        status.Charging = plt::getPinLevel(pins::chargeStatus) == false && c >= 10;
        
        
        status.batteryCapacity_percentage = (int)lipo.soc();                   
        status.batteryVoltage = (float)lipo.voltage() / 1000.0f;
        status.averageCurrent_ma = c;
        // unsigned int fullCapacity = lipo.capacity(FULL); // Read full capacity (mAh)
        // unsigned int capacity = lipo.capacity(REMAIN);   // Read remaining capacity (mAh)

        return true;
    }

   void HibernateSystem(){
      const uint64_t wakeup_pins = (1ULL << pins::powerBtn) | (1ULL << pins::clockAlarm);

      esp_sleep_enable_ext1_wakeup(wakeup_pins, ESP_EXT1_WAKEUP_ANY_LOW);

    //   rtc_gpio_pullup_en((gpio_num_t)pins::menuBtn);  // GPIO33 is tie to GND in order to wake up in HIGH
    //   rtc_gpio_pulldown_dis((gpio_num_t)pins::menuBtn); 

      // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
      // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
      // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
      esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
      esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

      //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);


        // prevent voltages from interacting with the display as the MCU browns out
        plt::setPinLevel(pins::dsp_CS, HIGH);
        plt::setPinLevel(pins::dsp_CLK, LOW);
        plt::setPinLevel(pins::dsp_SDI, LOW);
        plt::setPinLevel(pins::dsp_DC, LOW);
        plt::setPinLevel(pins::dsp_RST, LOW);
        plt::setPinMode(pins::dsp_CS, PinMode::Input);
        plt::setPinMode(pins::dsp_CLK, PinMode::Input);
        plt::setPinMode(pins::dsp_SDI, PinMode::Input);
        plt::setPinMode(pins::dsp_DC, PinMode::Input);
        plt::setPinMode(pins::dsp_RST, PinMode::Input);


      esp_deep_sleep_start();
   }
   
    void ShutdownSystem(){

        // prevent voltages from interacting with the display as the MCU browns out
        plt::setPinLevel(pins::dsp_CS, HIGH);
        plt::setPinLevel(pins::dsp_CLK, LOW);
        plt::setPinLevel(pins::dsp_SDI, LOW);
        plt::setPinLevel(pins::dsp_DC, LOW);
        plt::setPinLevel(pins::dsp_RST, LOW);
        plt::setPinMode(pins::dsp_CS, PinMode::Input);
        plt::setPinMode(pins::dsp_CLK, PinMode::Input);
        plt::setPinMode(pins::dsp_SDI, PinMode::Input);
        plt::setPinMode(pins::dsp_DC, PinMode::Input);
        plt::setPinMode(pins::dsp_RST, PinMode::Input);

        delay(50);

        plt::setPinMode(pins::powerKill, PinMode::Output);
        plt::setPinLevel(pins::powerKill, false);

        // should be unreachable when battery powered
        delay(100);
    }
   
   void LatchOnOffController(){
      plt::setPinLevel(pins::powerKill, true);
   }

   bool ConfigureFuelGauge(){

      // battery probably not connected
      if (!lipo.begin())
         return false;


      if (!lipo.itporFlag()) 
         return false;

      Serial.println("Writing gague config");

      bool success = true;

      success &= lipo.enterConfig();                
      success &= lipo.setCapacity(BatteryCapacity_mah); 

      /*
            Design Energy should be set to be Design Capacity × 3.7 if using the bq27441-G1A or Design
            Capacity × 3.8 if using the bq27441-G1B
      */
      success &= lipo.setDesignEnergy(BatteryCapacity_mah * 3.7f);

      /*
            Terminate Voltage should be set to the minimum operating voltage of your system. This is the target
            where the gauge typically reports 0% capacity
      */
      success &= lipo.setTerminateVoltage(BatteryTerminateVoltage_mv);

      /*
            Taper Rate = Design Capacity / (0.1 * Taper Current)
      */
      success &= lipo.setTaperRate(10 * BatteryCapacity_mah / TaperCurrent_ma);

      success &= lipo.exitConfig(); 
      
      return success;
   }
}