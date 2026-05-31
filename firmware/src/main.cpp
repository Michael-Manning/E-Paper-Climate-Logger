#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <driver/adc.h>
#include <stdint.h>

#include "MAssert.h"
#include "app.h"
#include "buttons.h"
#include "debugLog.h"
#include "drivers/DS3231.h"
#include "drivers/EEPROM.h"
#include "drivers/SHT45.h"
#include "drivers/display.h"
#include "pinout.h"
#include "platform.h"
#include "power.h"
// #include "timer_setter.h"
#include "Constants.h"
#include "bitmaps.h"

#include "esp_sleep.h"
#include <esp_timer.h>
#include "driver/rtc_io.h"
#include <Fonts/FreeMonoBold12pt7b.h>


bool debugLogUSBConnected = false;

unsigned long time_start_ms;
unsigned long time_now_s;
#define COLORED 0
#define UNCOLORED 1

using namespace plt;

namespace {
    
EEPROM eeprom;
DS3231 _clock;
Display _display;
App app;

GlobalState globalState = {};

uint32_t lightSleepInactivityTimer_ms = 0;
uint32_t deepSleepInactivityTimer_ms = 0;
uint32_t programStartTimer_ms = 0;
uint32_t clockReadTimer_ms = 0;

constexpr uint32_t lightSleepInactivity_ms = 1000;
constexpr uint32_t deepSleepInactivity_ms = 10000;

}  // namespace


void on_error(const char* cond, const char* file, int line) {
   Serial.printf("ERROR: assertion failed (%s) -- %s line: %d\n", cond, file, line);
}

void resetInactivity(){
    uint64_t now = millis(); 
    deepSleepInactivityTimer_ms  = now;
    lightSleepInactivityTimer_ms = now;
}

void configurePins() {
   setPinMode(pins::menuBtn, PinMode::Input_Pullup);
   setPinMode(pins::powerBtn, PinMode::Input);
   setPinMode(pins::powerKill, PinMode::Input);
   setPinMode(pins::USBDetect, PinMode::Input);
   setPinMode(pins::fuelGaugeGPIO, PinMode::Input);  // RIP
   setPinMode(pins::chargeStatus, PinMode::Input_Pullup);
   setPinMode(pins::clockAlarm, PinMode::Input);
}

void configureRTC() {
   DS3231::Control c;
   c.bits.disableOscillator = 0;
   c.bits.squareWaveEnable = 0;
   c.bits.convertTemperature = 0;
   c.bits.interruptControl = 1;
   c.bits.alarm2InterruptEnable = 0;
   c.bits.alarm1InterruptEnable = 1;
   _clock.SetControl(c);

   _clock.SetAlarm1(0);
}

void loadClimateDate(){
    int loadRange = 60;

    switch (globalState.currentSettings.graphRange)
    {
    case GraphRange::_10Minutes :
        loadRange = 10;
        break;
    case GraphRange::_20Minutes :
        loadRange = 20;
        break;
    case GraphRange::_30Minutes :
        loadRange = 30;
        break;
    case GraphRange::_60Minutes :
        loadRange = 60;
        break;
    default:
        break;
    }

    uint16_t requestAmount = globalState.currentHeader.dataCount < loadRange ? globalState.currentHeader.dataCount : loadRange; 

    uint16_t actualCount;
    eeprom.GetClimateSamples(
        globalState.currentHeader, 
        globalState.climateHistory.data(),  
        requestAmount,
        0,
        actualCount
    );
    globalState.historyEnd = 0;
    globalState.historyStart = actualCount;
}

void preHibernateTasks(){
    
    if(globalState.currentHeader.displayCold)
        _display.ShutDown();
    else
        _display.Hibernate();
    
    // write to eeprom
    logl("updating header");
    if(globalState.currentHeader.shutDownReason != ShutDownReason::FirmwareUpdate){
        globalState.currentHeader.shutDownReason = ShutDownReason::Hibernate;
    }
    eeprom.UpdateStatusHeader(globalState.currentHeader);

    if(power::USBConnected){
        logl("going back to sleep...");
        delay(200);
    }
}

// when woken up by alarm, just quickly update then screen then go back to sleep;
void doAlarmRoutine(bool updateDisplay){
    _clock.ClearAlarm1();
    if(updateDisplay)
        app.handle(globalState);
    preHibernateTasks();
    power::HibernateSystem();
}

void setup() {
    configurePins();
    power::LatchOnOffController();

    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
    bool wasHibernating = wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED;
    Wire.begin(pins::SDA, pins::SCL, 400000);

    // climate is priority because the MCU will heat up the board the longer it is on
    for (int i = 0; i < 5; i++)
    {
        if(globalState.lastClimateReading.humidity_raw != 0.0f)
            break;  
        getClimate(globalState.lastClimateReading); 
    }
    
    Serial.begin();
    debugLogUSBConnected = power::USBConnected();

    // need this delay or the first messages are not recieved by my computer
    if(debugLogUSBConnected && LoggerEnabled)
        delay(4000);
    
    if(wasHibernating)
        logl("Woke up from hibernation");
    else
        logl("Powered on");
    
    DateTime now;
    now = _clock.Read();
    
    // should be persistent, but it doesn't hurt to configure the alarm
    bool alarmWasSet = _clock.CheckAlarm();
    configureRTC();
    _clock.ClearAlarm1();
        
    // uncomment if a fresh board
    // power::ConfigureFuelGauge();
    PowerStatus ps;
    power::GetPowerStatus(ps);


    globalState.currentTime = _clock.Read();

    app.eeprom = &eeprom;
    app.disp = &_display;
    app.clock = &_clock;
    app.globalState = &globalState;
    app.setState(State::StandardDisplay);

    // figure out what's going on
    {
        bool shouldFullResetDisplay = true;
        bool validState = eeprom.ValidateHeaderState();
        if(!validState){
            logl("header state invalid. Resetting..");
            eeprom.FactoryReset();
            app.setState(State::InvalidStateNotice);
        }
        
        auto res = eeprom.GetLastStatusHeader(globalState.currentHeader);

        if(power::USBConnected()){
            delay(200);
        }
        else if(ps.batteryVoltage <= Constants::lowBatteryThreshold_V){
            globalState.currentHeader.shutDownReason = ShutDownReason::LowBattery;
            eeprom.UpdateStatusHeader(globalState.currentHeader);
            
            _display.ForceFullNextRefresh();
            _display.StartRefresh();
            _display.d->setRotation(1);
            _display.d->fillScreen(GxEPD_WHITE);
            _display.d->drawBitmap(0, 0, bitmaps::dead_battery_screen, 200, 200, GxEPD_BLACK);
            _display.EndRefresh();

            power::ShutdownSystem();
        }

        if(res == EEPROM::HeaderResult::ReadError){
            logl("failed to read EEPROM!");
        }
        else{
            if(res == EEPROM::HeaderResult::NoHeadersFound){
                logl("EEPROM is empty. Starting new file");
                globalState.currentHeader = FreshHeader();
                power::ConfigureFuelGauge();
                eeprom.WriteSettings(DefaultSettings());
            }
            else {
                if(globalState.currentHeader.shutDownReason == ShutDownReason::Hibernate){
                    logl("Last EEPROM header indicates hibernation");
                    globalState.currentHeader.shutDownReason = ShutDownReason::Unplanned;

                    // this was a healthy startup. Assume display is already initialized
                    shouldFullResetDisplay = false;
                }
                else if(globalState.currentHeader.shutDownReason == ShutDownReason::PowerOff || 
                    globalState.currentHeader.shutDownReason == ShutDownReason::LowBattery){

                    logl("Last EEPROM header indicates power cycle. Starting fresh header");
                    globalState.currentHeader = FreshHeader();
                    app.setState(State::WelcomeScreen);
                }
                else if(globalState.currentHeader.shutDownReason == ShutDownReason::FirmwareUpdate){
                    logl("Last EEPROM header indicates firmware update.");
                    globalState.currentHeader.shutDownReason = ShutDownReason::FirmwareUpdate;
                    eeprom.WriteSettings(DefaultSettings());
                }
                else if(globalState.currentHeader.shutDownReason == ShutDownReason::Unplanned){
                    logl("Last EEPROM header indicates unplanned shutdown");
                    app.setState(State::InvalidStateNotice);
                    globalState.currentHeader = FreshHeader();
                    eeprom.WriteSettings(DefaultSettings());
                }
                else{
                    MASSERT(false);
                    logf("unknown shutdown reason: %d", (int)globalState.currentHeader.shutDownReason);
                    eeprom.WriteSettings(DefaultSettings());
                }
            }

            logl("starting header");
            if(power::USBConnected())
                delay(200);
            bool success = eeprom.StartNextStatusHeader(globalState.currentHeader);
            MASSERT(success);
            

            logl("updating header");
            if (power::USBConnected())
               delay(200);
            success = eeprom.UpdateStatusHeader(globalState.currentHeader);
            MASSERT(success);
        }
        
        eeprom.ReadSettings(globalState.currentSettings);

        if(globalState.currentHeader.dataCount == 0 || alarmWasSet){
            eeprom.AddClimateSample(globalState.currentHeader, globalState.lastClimateReading);
        }

        bool displayCold = globalState.lastClimateReading.Temperature_c() < Constants::lowTemperatureDisplayThreshold_C;
        bool displayingColdWarningScreen = false; // previus power cycle indicates the device screen has been too cold for some time already.
        if(displayCold){
            if(globalState.currentHeader.displayCold == false){
                globalState.currentHeader.displayCold = true;
                app.setState(State::LowTempWarning);
            }
            else{
                displayingColdWarningScreen = true; // can assume the display is already showing an underheat warning
            }
        }
        else {
            // reset the header flag once it warms back up
            if(globalState.currentHeader.displayCold == true){
                globalState.currentHeader.displayCold = false;
            }
        }

        _display.Init(shouldFullResetDisplay, &globalState.currentHeader);

        if(wasHibernating){
            if(alarmWasSet){
                logl("woke up due to alarm");
                
                if(power::USBConnected() == false){
                    // we woke up from our regular scheduled minute alarm. Do the normal routine and go back to sleep.
                    globalState.lastDeepWakeupCause = DeepWakeupCause::Alarm;
                    globalState.sleepIndicator = true;
                    
                    if(!displayingColdWarningScreen)
                        loadClimateDate();  

                    doAlarmRoutine(!displayingColdWarningScreen);
                    
                    // unreachable code due to deep sleep
                    return;
                }
                else if (
                    globalState.currentHeader.shutDownReason != ShutDownReason::FirmwareUpdate && 
                    globalState.currentHeader.shutDownReason != ShutDownReason::Unplanned && 
                    LoggerEnabled == false){

                    if(ps.Charging)
                        app.setState(State::ChargingScreen);
                }
            }
            else{
                globalState.lastDeepWakeupCause = DeepWakeupCause::ButtonFromHibernate;
                globalState.sleepIndicator = false;
                logl("woke up due to button press");
            }
        }
        else{

            if(alarmWasSet){
                // unlikely, but we don't want to take two climate samples if this is the case
                _clock.ClearAlarm1();
            }

            if(globalState.currentHeader.shutDownReason == ShutDownReason::FirmwareUpdate)
                globalState.lastDeepWakeupCause = DeepWakeupCause::FirmwareUpdate;
            else
                globalState.lastDeepWakeupCause = DeepWakeupCause::ButtonFromPowerOff;
        }

        // woke for reasons other than alarm, but too cold to drive the display. 
        // No point in doing anything so just go back to sleep
        if(displayCold){
            if(!displayingColdWarningScreen){
                // this is the first update since the temperature crossed the cold threshold, so we do a one time update to show the cold warning
                app.handle(globalState);
            }
            preHibernateTasks();
            power::HibernateSystem();
        }
    }

    loadClimateDate();

    resetInactivity();
    programStartTimer_ms = millis();
}

constexpr uint8_t FirmwareUpdateNotifyByte = 0xC1;
bool downloadingFirmware = false;
void pollUSB() {
    if (Serial.available()) {

        int c = Serial.peek();
        
        // legacy: update the time over usb using timer sender application
        // if(c == TimeMessageStartByte){

        //     uint8_t hh, mm, ss;
        //     if (readTimeMessage(Serial, hh, mm, ss)) {
                
        //         DateTime dt;
        //         dt.Hour = hh;
        //         dt.Minute = mm;
        //         dt.Second = ss;
        //         _clock.Write(dt);
                
        //         char buf[20];
        //         GetTimeString24(buf, dt);
                
        //         Serial.printf("Time received: %s :D\n", buf);
                
        //         Serial.println(ss);
        //     }
        // }
        /*else*/ if(c == FirmwareUpdateNotifyByte){
            globalState.currentHeader.shutDownReason = ShutDownReason::FirmwareUpdate;
            eeprom.UpdateStatusHeader(globalState.currentHeader);

            _display.StartRefresh();
            _display.d->setRotation(2);
            _display.d->setFont(&FreeMonoBold12pt7b);
            _display.d->fillScreen(GxEPD_WHITE);
            _display.d->setCursor(0, 96);
            _display.d->print("downloading\nfirmware...");
            _display.EndRefresh();

            Serial.read();
            Serial.printf("Update notification received\n");

            downloadingFirmware = true; 
        }
    }
}

static inline uint64_t now_rtc_ms() {
    return (uint64_t)esp_timer_get_time() / 1000ULL; // includes light sleep time
}

// sleep until a button is pressed, a clock alarm, or screen hibernation timer expires
void doLightSleep(){

    rtc_gpio_pullup_en((gpio_num_t)pins::menuBtn);
    rtc_gpio_pulldown_dis((gpio_num_t)pins::menuBtn);

    esp_sleep_enable_ext1_wakeup(
        (1ULL << pins::powerBtn) | (1ULL << pins::menuBtn) | (1ULL << pins::clockAlarm), 
        ESP_EXT1_WAKEUP_ANY_LOW);

    // Configure timer wakeup after deep sleep timeout to go into a deeper sleep state
    esp_sleep_enable_timer_wakeup((deepSleepInactivity_ms - lightSleepInactivity_ms) * 1000); // in microseconds

    if(power::USBConnected()){
        logl("Entering light sleep...");
        delay(100); // Ensure message is printed
    }

    const uint64_t sleep_start_ms = now_rtc_ms();
    esp_light_sleep_start();
    const uint64_t slept_ms = now_rtc_ms() - sleep_start_ms;

    rtc_gpio_pullup_dis((gpio_num_t)pins::menuBtn);

    bool btn1State = !getPinLevel(pins::menuBtn);
    bool btn2State = !getPinLevel(pins::powerBtn);

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:{

        if (btn1State) 
            globalState.lastLightWakeupCause = LightWakeupCause::MenuButton;
        else if (btn2State) 
            globalState.lastLightWakeupCause = LightWakeupCause::PowerButton;
        else if(_clock.CheckAlarm())
            globalState.lastLightWakeupCause = LightWakeupCause::ClockAlarm;
        else
            globalState.lastLightWakeupCause = LightWakeupCause::Unknown_ext1;
        break;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
        globalState.lastLightWakeupCause = LightWakeupCause::Timer;
        break;
    default:
        globalState.lastLightWakeupCause = LightWakeupCause::Unknown;
        break;
    }

    // compensate for millis() freezing during light sleep and advance deep sleep timer
    if (slept_ms > 0) {
        if (slept_ms > deepSleepInactivityTimer_ms) 
            deepSleepInactivityTimer_ms = 0;
        else 
            deepSleepInactivityTimer_ms -= slept_ms;
    }

    // hard reset debounce because timer has been paused due to sleep
    debounceButton1(btn1State, millis(), true); 

    // clear all held, multipress state;
    updateButtonState(false, false, millis());

    // check if light wakeup could just be a coincidence. Not user activity
    if(
        globalState.lastLightWakeupCause != LightWakeupCause::ClockAlarm &&
        globalState.lastLightWakeupCause != LightWakeupCause::Timer
    ){
        resetInactivity();
    }
}

uint32_t shutdownTimer = 0;
bool shuttingDown = false;
bool firstLoop = true;

void loop() {
    debugLogUSBConnected = power::USBConnected();

    bool btn1Filtered = debounceButton1(!getPinLevel(pins::menuBtn), millis(), false);
    updateButtonState(
        btn1Filtered,
        !getPinLevel(pins::powerBtn), // btn2 is already debounced by the LTC2954
        millis());
        
    // firmware updates cause multiple resets, so wait before reseting the shutdown reason
    if(globalState.currentHeader.shutDownReason == ShutDownReason::FirmwareUpdate){
        if(power::USBConnected()){
            if(millis() - programStartTimer_ms > 28000){
                globalState.currentHeader.shutDownReason = ShutDownReason::Unplanned;
                eeprom.UpdateStatusHeader(globalState.currentHeader);
            }
        }
        else{
            globalState.currentHeader.shutDownReason = ShutDownReason::Unplanned;
            eeprom.UpdateStatusHeader(globalState.currentHeader);
        }
    }

    if(firstLoop && getBtn2Down() == false && globalState.lastDeepWakeupCause == DeepWakeupCause::ButtonFromHibernate){
        // It's possible to wake up the device via button press and then release it quickly enough before it gets
        // registered by the button state manager. So if we just woke up and we know the button was just pressed according
        // to the wakeup source, we should still perform the button action (which is to enter the main menu)
        app.setState(State::MainMenu);
    }

    if(getBtn1() || getBtn2()){ 
        resetInactivity();
    }

    // poll the RTC once per second
    if(millis() - clockReadTimer_ms > 1000){
        clockReadTimer_ms = millis();
        globalState.currentTime = _clock.Read();
    }

    // handle manual shut down
    if (getBtn2HeldMS() >= 3000 && shuttingDown == false) {

        // Note:
        // The LTC2954 is configured to cut power after the button is held down for 6 seconds.
        // So we are effectivelly limited to 6 seconds to close up shop and shut down gracefully
        // before the LTC2954 activates its failsafe.

        globalState.currentHeader.shutDownReason = ShutDownReason::PowerOff;
        eeprom.UpdateStatusHeader(globalState.currentHeader);

        logl("shutting down...");

        shuttingDown = true;
        shutdownTimer = millis() - 3000;
        app.setState(State::ShutDownScreen);
    }

    if (shuttingDown && millis() - shutdownTimer >= 4000) {
        _display.FullClear();
        _display.ShutDown();

        if(power::USBConnected()){
            Serial.printf("powered down - %d\n", (int)(millis() - shutdownTimer));
            delay(100);
        }
        power::ShutdownSystem();

        // shouldn't ever get to this point unless powered by USB and a mistake was made. Shutting down isn't allowed while plugged in.
        while (shutdownTimer < 6500 && getPinLevel(pins::powerBtn) == true) {
            delay(1);
        }

        // That didn't work. I guess we'll just hibernate and pretend to be asleep? At least that way it will require a button press to turn back on
        shuttingDown = false;
        power::HibernateSystem();
    }

    // handle RTC alarm while already awake
    if (_clock.CheckAlarm()) {
        logl("alarm triggered");
        _clock.ClearAlarm1();

        for (int i = 0; i < 5; i++)
        {
            if(globalState.lastClimateReading.humidity_raw != 0.0f)
                break;  
            getClimate(globalState.lastClimateReading); 
        }

        eeprom.AddClimateSample(globalState.currentHeader, globalState.lastClimateReading);
 
        if(app.getState() == State::StandardDisplay){
            app.setState(State::StandardDisplay); // refresh screen to reflect new time and temperature
        }
    }

    if(power::USBConnected()){
        pollUSB();

        if(downloadingFirmware == false && LoggerEnabled == false){
            if(app.getState() != State::ChargingScreen && app.getState() != State::InvalidStateNotice){
                app.setState(State::ChargingScreen);
            }
        }
    }
    else if(app.getState() == State::ChargingScreen)
    {
        // stop showing charging screen immedidately when unplugged
        resetInactivity();
        _display.EnableFullRefresh();
        app.setState(State::StandardDisplay);
    }


    // deep sleep due to inactivity
    if(
        !power::USBConnected() && !shuttingDown && 
        (millis() - deepSleepInactivityTimer_ms > deepSleepInactivity_ms || globalState.shouldGoToSleep))
    {
        _display.EnableFullRefresh();
        globalState.sleepIndicator = true;
        app.setState(State::StandardDisplay);
        app.handle(globalState);
        preHibernateTasks();
        power::HibernateSystem();

        // unreachable code due to deep sleep
    }

    // light sleep due to inactivity
    if(!shuttingDown && millis() - lightSleepInactivityTimer_ms > lightSleepInactivity_ms){
        _display.Hibernate();
        if(!power::USBConnected())
            doLightSleep();
    }

    app.handle(globalState);

    firstLoop = false;
}
