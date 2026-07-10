#pragma once

#include "Constants.h"

struct PowerStatus{
   bool Charging;
   float batteryVoltage = 0.0f;
   int batteryCapacity_percentage = 0;
   int averageCurrent_ma =0;
};

namespace power{

    constexpr uint16_t BatteryCapacity_mah = 400;
    // 10% of ISET-programmed charge current: 890 / 5100Ω = 174.5mA → taper = 17.5mA
    constexpr uint16_t TaperCurrent_ma = 17;

    bool USBConnected();

    bool ChargeStatus();

    bool GetPowerStatus(PowerStatus& status);

    void HibernateSystem();

    void ShutdownSystem();

    void LatchOnOffController();

    bool ConfigureFuelGauge();

    bool BatteryReconnected();
}
