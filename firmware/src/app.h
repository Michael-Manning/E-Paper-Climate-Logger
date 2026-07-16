#pragma once

#include <array>
#include <functional>

#include "drivers/display.h"
#include "drivers/DS3231.h" 
#include "drivers/SHT45.h"
#include "drivers/EEPROM.h"
#include "debugLog.h"
#include "buttons.h"

enum DeepWakeupCause{
    ButtonFromHibernate,
    ButtonFromPowerOff,
    FirmwareUpdate,
    Alarm,
    FuelGuageGPIOFromHibernate,
};
enum LightWakeupCause{
    PowerButton,
    MenuButton,
    Timer,
    ClockAlarm,
    FuelGuageGPIO,
    NA,
    Unknown,
    Unknown_ext1
};

struct GlobalState{
    ClimateSample lastClimateReading;
    DateTime currentTime = {};
    StatusHeader currentHeader;
    DeepWakeupCause lastDeepWakeupCause;
    LightWakeupCause lastLightWakeupCause = LightWakeupCause::NA;
    Settings currentSettings;

    std::array<ClimateSample, flash::dataCapacity> climateHistory;
    uint16_t historyStart = 0; // in minutes ago
    uint16_t historyEnd = 0; // in minutes ago

    bool sleepIndicator = false;
    bool shouldGoToSleep = false; // hint that we don't need to wait for innactivity to enter deep sleep
};

enum class State {
   StandardDisplay,
   MainMenu,
   InvalidStateNotice,
   ShutDownScreen,
   DebugMenu,
   ViewData,
   LowTempWarning,
   TimeSet,
   ChargingScreen,
   SettingsMenu,   
   WelcomeScreen, 
   DownloadingUpdate,  
   UpdateComplete,
   NightMode,
   EReader,
   Nothing,
   Count
};

extern void resetInactivity();

class App{
public:
    App() {
        handlers = {
            [this](){ StandardDisplay(); },
            [this](){ MainMenu(); },
            [this](){ InvalidStateNotice(); },
            [this](){ ShutDownScreen(); },
            [this](){ DebugMenu(); },
            [this](){ ViewData(); },
            [this](){ LowTempWarning(); },
            [this](){ TimeSet(); },
            [this](){ ChargingScreen(); },
            [this](){ SettingsMenu(); },
            [this](){ WelcomeScreen(); },
            [this](){ DownloadingUpdate(); },
            [this](){ UpdateComplete(); },
            [this](){ NightMode(); },
            [this](){ EReader(); },
            [this](){  },
        };
    }

    void handle(const GlobalState& globalState);

    void setState(State newState){

        if(newState == State::MainMenu)
            disp->DisableFullRefresh();

        if(newState == State::ViewData) {
            viewDataHourOffset = 0;
            viewDataShowHumidity = false;
        }

        if(currentState == State::ChargingScreen && newState != State::ChargingScreen) {
            chargingScreenBatteryPct = -1;
            chargingScreenBatteryVoltage_V = -1.0f;
        }

        clearMultipressState();

        logf("set screen %d\n", (int)newState);
        currentState = newState; 
        redraw = true;
    }
    State getState(){
        return currentState;
    };
   
    EEPROM* eeprom;
    Display* disp;
    DS3231* clock;
    GlobalState * globalState;

private:
   
    bool redraw = false;

    bool shouldRedraw(){
        bool temp = redraw;
        redraw = false;
        return temp;
    };
    void setRedraw(){
        redraw = true;
    };

    DateTime currentTime;


    void StandardDisplay();
    void MainMenu();
    void InvalidStateNotice();
    void ShutDownScreen();
    void DebugMenu();
    void ViewData();
    void LowTempWarning();
    void TimeSet();
    void ChargingScreen();
    void SettingsMenu();
    void WelcomeScreen();
    void DownloadingUpdate();
    void UpdateComplete();
    void NightMode();
    void EReader();

    void drawGraph(int left, int right, int bottom, int top, const int16_t* dataY, int dataCount, int16_t* min, int16_t* max, bool forceRange = false, int16_t forcedMin = 0, int16_t forcedMax = 0);

    State currentState = State::StandardDisplay;
    std::array<std::function<void()>, static_cast<size_t>(State::Count)> handlers;


    // just a slop section with any random UI state we need
    int menuSelectedOption = 0;
    int selectedSetting = 0;
    int timeSetStage = 0;
    int timeSetHour = 0;
    int timeSetMinute = 0;
    uint64_t updateCompleteEntryTime_ms = 0;
    int viewDataHourOffset = 0;
    bool viewDataShowHumidity = false;
    int chargingScreenBatteryPct = -1;
    float chargingScreenBatteryVoltage_V = -1.0f;
    uint32_t ebookIndex = 0; // index of the of the first character of the current page.

};