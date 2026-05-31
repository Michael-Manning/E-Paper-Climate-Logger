#include <stdint.h>

#include <hal/gpio_hal.h>
#include <driver/gpio.h>

#include "platform.h"

namespace plt {

    void setPinMode(int pin, PinMode mode) {

        gpio_hal_context_t gpiohal;
        gpiohal.dev = GPIO_LL_GET_HW(GPIO_PORT_0);

        gpio_config_t conf = {
            .pin_bit_mask = (1ULL << pin),             
            .mode = GPIO_MODE_DISABLE,                  
            .pull_up_en = GPIO_PULLUP_DISABLE,          
            .pull_down_en = GPIO_PULLDOWN_DISABLE,     
            .intr_type = static_cast<gpio_int_type_t>(gpiohal.dev->pin[pin].int_type) 
        };

        if(mode >= PinMode::Input && mode <= PinMode::Input_Pullup){
                if(mode == PinMode::Input_Pulldown){
                    conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
                }
                else if(mode == PinMode::Input_Pullup){
                    conf.pull_up_en = GPIO_PULLUP_ENABLE;
                }
            conf.mode = static_cast<gpio_mode_t>(static_cast<int>(conf.mode) | GPIO_MODE_DEF_INPUT);
        }


        else if(mode == PinMode::Output){
            conf.mode = static_cast<gpio_mode_t>(static_cast<int>(conf.mode) | GPIO_MODE_DEF_OUTPUT);
        }

        gpio_config(&conf);
    }

    void setPinLevel(int pin, uint32_t level){
        gpio_set_level(static_cast<gpio_num_t>(pin), level);
    }

    int getPinLevel(int pin){
        return gpio_get_level(static_cast<gpio_num_t>(pin));
    }

}  