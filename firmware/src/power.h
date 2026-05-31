#pragma once

struct PowerStatus{
   bool Charging;
   float batteryVoltage = 0.0f;
   int batteryCapacity_percentage = 0;
   int averageCurrent_ma =0;
};

namespace power{

    constexpr uint16_t BatteryCapacity_mah = 400;
    constexpr uint16_t BatteryTerminateVoltage_mv = 3300;
    constexpr uint16_t TaperCurrent_ma = 12; // VERIFY

    bool USBConnected();

    bool GetPowerStatus(PowerStatus& status);

    void HibernateSystem();

    void ShutdownSystem();

    void LatchOnOffController();

    bool ConfigureFuelGauge();

}
