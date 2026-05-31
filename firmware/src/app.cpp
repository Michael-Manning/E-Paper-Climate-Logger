#include <stdint.h>
#include <Arduino.h>

#include "drivers/display.h"
#include "drivers/DS3231.h"
#include "buttons.h"
#include "power.h"
#include "app.h"

#include "bitmaps.h"
#include "Constants.h"

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>


namespace{
    constexpr uint16_t black = GxEPD_BLACK;
    constexpr uint16_t white = GxEPD_WHITE;
    constexpr int dispw = 199;
    constexpr int disph = 199;

    std::array<int16_t, flash::dataCapacity> rawTemperatureBuffer;
}

void App::handle(const GlobalState& globalState) {
      this->currentTime = globalState.currentTime;
      handlers[static_cast<size_t>(currentState)](); 
}

uint8_t _buffer[200 / 8 * 200];

void plotLineWidthBW(int x0, int y0, int x1, int y1, float wd, GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> *d)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, e2, x2, y2;

    // normalization factor
    float ed = (dx + dy == 0) ? 1.0f : sqrtf((float)dx * (float)dx + (float)dy * (float)dy);

    // half-width
    wd = (wd + 1.0f) / 2.0f;

    while(true)
    {
        // center pixel
        d->writePixel(x0, y0, black);

        e2 = err;
        x2 = x0;

        // x step
        if (2 * e2 >= -dx)
        {
            // vertical thickening along the normal direction
            for (e2 += dy, y2 = y0; e2 < ed * wd && (y1 != y2 || dx > dy); e2 += dx)
                d->writePixel(x0, (y2 += sy), black);

            if (x0 == x1) 
                break;
            e2 = err;
            err -= dy;
            x0 += sx;
        }

        // y step
        if (2 * e2 <= dy)
        {
            // horizontal thickening along the normal direction.
            for (e2 = dx - e2; e2 < ed * wd && (x1 != x2 || dx < dy); e2 += dy)
                d->writePixel((x2 += sx), y0, black);

            if (y0 == y1) 
                break;
            err += dx;
            y0 += sy;
        }
    }
}


static inline int mapY_to_screen(int v, int ymin, int height, int bottom, int yrange, int invb)
{
    // yrange == 0 → flat line centered between bottom and top
    if (yrange == 0) {
        const int mid_from_bottom = bottom + (height >> 1);
        return disph - mid_from_bottom;
    }
    // fixed-point scale: Q16 (height * 2^16 / yrange)
    const int64_t scaleQ16 = ((int64_t)height << 16) / yrange;
    // t_pixels = (v - ymin) * scaleQ16 >> 16   (rounded)
    const int64_t dy = (int64_t)v - (int64_t)ymin;
    const int t_pixels = (int)((dy * scaleQ16 + (1 << 15)) >> 16);
    const int y_from_bottom = bottom + t_pixels;
    return disph - y_from_bottom;
}

// y coordinates including bottom,top are from physical bottom of the screen
void App::drawGraph(int left, int right, int bottom, int top, const int16_t* dataY, int dataCount, int* min, int* max){
    auto d = disp->d;
    uint32_t timeStamp = micros();
    

    if (left >= right || bottom >= top) 
        return;

    const int width  = right - left;
    const int height = top - bottom;

    // convert graph box to screen coordinates once
    const int invt = disph - top;
    const int invb = disph - bottom;

    // outline
    d->drawLine(left,  invt,  right, invt,  black);
    d->drawLine(left,  invb,  right, invb,  black);
    d->drawLine(left,  invt,  left,  invb,  black);
    d->drawLine(right, invt,  right, invb, black);
    
    // grid
    for (int i = 1; i < width - 1; i++)
    {
        uint16_t col = i % 2 ? black : white;
        d->drawPixel(left + i, invt+(height / 2), col);
        d->drawPixel(left + i, invt+(height / 4), col);
        d->drawPixel(left + i, invb-(height / 4), col);
    }
    for (int i = 1; i < height - 1; i++)
    {
        uint16_t col = i % 2 ? black : white;
        d->drawPixel(left + width/2, invt + i, col);
        d->drawPixel(left + width/4, invt + i, col);
        d->drawPixel(right - width/4, invt + i, col); 
    }

    if (!dataY || dataCount < 2) 
        return;

    // min/max
    int ymin = dataY[0], ymax = dataY[0];
    for (int i = 1; i < dataCount; ++i) {
        const int v = dataY[i];
        if (v < ymin) ymin = v;
        if (v > ymax) ymax = v;
    }
    const int yrange = ymax - ymin;

    *max = ymax;
    *min = ymin;

    // fast path: #samples <= #pixels -> draw polyline
    if (dataCount <= (width + 1)) {
        // X stepping via fixed-point accumulator (Q16)
        const int64_t xStepQ16 = (dataCount > 1) ? (((int64_t)width << 16) / (dataCount - 1)) : 0;
        int64_t fx = 0;

        int prevX = left;
        int prevY = mapY_to_screen(dataY[0], ymin, height, bottom, yrange, invb);

        for (int i = 1; i < dataCount; ++i) {
            fx += xStepQ16;
            const int x = left + (int)((fx + (1 << 15)) >> 16);
            const int y = mapY_to_screen(dataY[i], ymin, height, bottom, yrange, invb);
            // d->drawLine(prevX, prevY, x, y, black);
            plotLineWidthBW(prevX, prevY, x, y, 1.5f, d);
            prevX = x;
            prevY = y;
        }
        logf("graph render: %dus\n", micros() - timeStamp);
        return;
    }


    // slow path: #samples > #pixels -> bucket-average per X pixel
    {
        const int64_t xStepQ16 = ((int64_t)width << 16) / (dataCount - 1);
        int64_t fx = 0;

        int curX = left + (int)((fx + (1 << 15)) >> 16);
        int64_t sumY = 0; 
        int cnt  = 0;

        int prevX = -1;
        int prevY = 0;

        for (int i = 0; i < dataCount; ++i) {
            const int x = left + (int)((fx + (1 << 15)) >> 16);

            // new pixel column -> flush previous bucket
            if (x != curX) {
                if (cnt > 0) {
                    const int avgYsrc = (int)(sumY / cnt);
                    const int y = mapY_to_screen(avgYsrc, ymin, height, bottom, yrange, invb);
                    if (prevX >= 0) {
                        plotLineWidthBW(prevX, prevY, curX, y, 1.5f, d);
                    }
                    prevX = curX;
                    prevY = y;
                }
                curX = x;
                sumY = 0;
                cnt  = 0;
            }

            sumY += (int)dataY[i];
            cnt  += 1;
            fx   += xStepQ16;
        }

        // flush last bucket
        if (cnt > 0) {
            const int avgYsrc = (int)(sumY / cnt);
            const int y = mapY_to_screen(avgYsrc, ymin, height, bottom, yrange, invb);
            if (prevX >= 0) {
                plotLineWidthBW(prevX, prevY, curX, y, 1.5f, d);
            }
            prevX = curX;
            prevY = y;
        }

        // close any small gap to the right edge for visual completeness
        if (prevX >= 0 && prevX < right) {
            plotLineWidthBW(prevX, prevY, right, prevY, 1.5f, d);
        }
    }


    logf("graph render: %dus\n", micros() - timeStamp);
}

void App::StandardDisplay(){
   auto d = disp->d;

    if(shouldRedraw()){
        disp->StartRefresh();
        d->setRotation(2);
        d->fillScreen(white);
        
        int readingCount = globalState->historyStart - globalState->historyEnd;

        d->drawBitmap(2, 0, bitmaps::thermometer32, 12, 32, black);

        // draw temperature value, degree symbol, and arror
        {
            char tempstr[8];

            if(globalState->currentSettings.tempFormat == TemperatureFormat::Celcius){
                
                // separate integer and decimal digits with proper rounding
                int iv;
                int dv;
                int t100 = globalState->lastClimateReading.temperature_raw;
                {
                    int32_t t10 = (t100 >= 0) ? (t100 + 5) / 10   // +0.05 then truncate
                                          : (t100 - 5) / 10;  // -0.05 then truncate
                                          
                                          bool neg = (t10 < 0);
                                          uint32_t at10 = neg ? static_cast<uint32_t>(-t10)
                                    : static_cast<uint32_t>( t10);
                    iv = at10 / 10;
                    dv = at10 % 10;
                }
                sprintf(tempstr, "%d", iv);
                d->setFont(&FreeMonoBold24pt7b);
                disp->printf(18, 29, "%d", iv);
                int16_t x1,y1;
                uint16_t w,h;
                d->getTextBounds(tempstr, 18, 28, &x1, &y1, &w, &h);
                disp->print(x1 + w - 7, 29, ".");
                disp->printf(x1 + w + 12, 29, "%d", dv);    
                d->drawBitmap(x1 + w + 40, 2, bitmaps::degree_symbol, 11, 11, black );
            }
            else{ 
                // fahrenheit
                int iv = (int)roundf(globalState->lastClimateReading.Temperature_f());
                sprintf(tempstr, "%d", iv);
                d->setFont(&FreeMonoBold24pt7b);
                disp->printf(18, 29, "%d", iv);
                int16_t x1,y1;
                uint16_t w,h;
                d->getTextBounds(tempstr, 18, 28, &x1, &y1, &w, &h);
                d->drawBitmap(x1 + w + 3, 2, bitmaps::degree_symbol, 11, 11, black );
            }
            
            if(readingCount > 1){
                int16_t cur = globalState->climateHistory[globalState->historyStart - 1].temperature_raw;
                int16_t last = globalState->climateHistory[globalState->historyStart - 2].temperature_raw;
                if(cur > last)
                d->drawBitmap(128, 0, bitmaps::up_arrow, 15, 24, black);
                else if(cur < last)
                d->drawBitmap(128, 0, bitmaps::down_arrow, 15, 24, black);
            }
        }

        
        // reaction emjoi
        {
            float currentTemp_C = globalState->lastClimateReading.Temperature_c();
            const uint8_t* emoji;
            if     (currentTemp_C < -10)emoji = bitmaps::face_scary_2;
            else if(currentTemp_C < 0)  emoji = bitmaps::face_scary_2;
            else if(currentTemp_C < 2)  emoji = bitmaps::face_scary_1;
            else if(currentTemp_C < 4)  emoji = bitmaps::face_death_1;
            else if(currentTemp_C < 10) emoji = bitmaps::face_cold_1;
            else if(currentTemp_C < 12) emoji = bitmaps::face_sad_1;
            else if(currentTemp_C < 14) emoji = bitmaps::face_neutral_1;
            else if(currentTemp_C < 16) emoji = bitmaps::face_neutral_2;
            else if(currentTemp_C < 18) emoji = bitmaps::face_neutral_3;
            else if(currentTemp_C < 20) emoji = bitmaps::face_happy_4;
            else if(currentTemp_C < 24) emoji = bitmaps::face_happy_2; // 22
            //else if(currentTemp_C < 24) emoji = bitmaps::face_happy_1;
            else if(currentTemp_C < 26) emoji = bitmaps::face_happy_3;
            else if(currentTemp_C < 27) emoji = bitmaps::face_hot_1;
            else if(currentTemp_C < 28) emoji = bitmaps::face_hot_2;
            else if(currentTemp_C < 31) emoji = bitmaps::face_hot_3;
            else if(currentTemp_C < 33) emoji = bitmaps::face_hot_4;
            else if(currentTemp_C < 35) emoji = bitmaps::face_scary_1;
            else if(currentTemp_C < 39) emoji = bitmaps::face_scary_2;
            else                      emoji = bitmaps::face_empty_1;
            
            d->drawBitmap(151, 0, emoji, 48, 48, black);
        }
        
        d->setFont(&FreeMonoBold12pt7b);
        {
            float currentHumidity = globalState->lastClimateReading.Humidity_r();
            d->drawBitmap(0, 37, bitmaps::water_drop, 17, 17, black);
            char tempstr[8];
            sprintf(tempstr, "%d", (int)roundf(currentHumidity));
            int16_t x1,y1;
            uint16_t w,h;
            d->getTextBounds(tempstr, 20, 52, &x1, &y1, &w, &h);
            disp->printf(20, 52, "%d", (int)roundf(currentHumidity));
            d->drawBitmap(x1 + w + 3, 38, bitmaps::percent_symbol_16, 16, 16, black);

            d->drawBitmap(0, 58, bitmaps::clock, 17, 17, black);
            char timebuf[12];
            if(globalState->currentSettings.timeFormat == TimeFormat::_12Hour)
                GetTimeString12(timebuf, currentTime);
            else
                GetTimeString24(timebuf, currentTime);
            disp->print(20, 73, timebuf);
        }

        for (int i = globalState->historyEnd; i < globalState->historyStart; i++)
        {
            rawTemperatureBuffer[i - globalState->historyEnd] = globalState->climateHistory[i].temperature_raw;
            // logf("%d ", (int)globalState->climateHistory[i].temperature_raw);
        }
        logf("\n");

        int min, max;
        drawGraph(0, 199, 13, 118, rawTemperatureBuffer.data(), readingCount, &min, &max);

        d->setFont(&FreeMonoBold9pt7b);
        if(readingCount > 1){
            disp->printf(0, 199, "T-%dm", (int(readingCount)));
            if(globalState->sleepIndicator && Constants::sleepIndicator)
                disp->print(100, 199, "s");
            // disp->printf(100, 199, "%d", globalState->currentHeader.partialRefreshCount);
                
            ClimateSample mins = {.temperature_raw = min};
            ClimateSample maxs = {.temperature_raw = max};
            if(globalState->currentSettings.tempFormat == TemperatureFormat::Celcius){
                disp->printf(140, 199, "%.2f", mins.Temperature_c());
                disp->printf(140, 79, "%.2f", maxs.Temperature_c());
            }
            else{
                disp->printf(140, 199, "%.1f", mins.Temperature_f());
                disp->printf(140, 79, "%.1f", maxs.Temperature_f());
            }
        }
        else{
            disp->printf(0, 199, "No data", (int(readingCount)));
        }

        disp->EndRefresh();
    }

    if(getBtn2Down()){
        setState(State::MainMenu);
        menuSelectedOption = 0;

        resetInactivity();
    }
}

void App::MainMenu(){
    auto d = disp->d;

    const char* options[] = {
        "View data",
        "Debug menu",
        "Settings",
        "Set time"
    };
    const int optionsCount = 4;
    
    if(shouldRedraw()){
        disp->StartRefresh();
        d->setRotation(1);
        d->fillScreen(white);

        // top bar
        {
            d->setFont(&FreeMonoBold9pt7b);

            d->drawCircle(14, 6, 2, black);
            disp->print(24, 10, "Back");
            
            
            d->drawCircle(120, 6, 2, black);
            disp->print(128, 10, "Next");

            d->drawCircle(110, 22, 2, black);
            d->drawCircle(120, 22, 2, black);
            disp->print(128, 26, "Enter");
            
            d->drawFastHLine(0, 30, 199, black);
        }

        d->setFont(&FreeMonoBold12pt7b);
        
        int optionsPos = 50;
        for (int i = 0; i < optionsCount; i++)
        {
            
            int y = optionsPos + 20 * i;
            int xo = disp->printAndOffset(0, y, options[i]);
            int gap = 6;
            if(menuSelectedOption == i)
                d->drawFastHLine(xo + gap, y - 6, dispw - xo - gap, black);
        }
        
        disp->EndRefresh();
    }
    
    if(getBtn2Down()){
        setState(State::StandardDisplay);
        globalState->shouldGoToSleep = true;
    }
    if(getBtn1MultiPressConfirmed() == 1){
        menuSelectedOption = (menuSelectedOption + 1) % optionsCount;
        setState(State::MainMenu);
    }
    else if(getBtn1MultiPressimmediate() == 2){
        if(menuSelectedOption == 0){
            setState(State::ViewData);
        }
        if(menuSelectedOption == 1){
            setState(State::DebugMenu);
        }
        if(menuSelectedOption == 2){
            setState(State::SettingsMenu);
            selectedSetting = 0;
        }
        if(menuSelectedOption == 3){
            setState(State::TimeSet);
            timeSetStage = 0;
        }
    }
}

void App::ShutDownScreen() 
{
    auto d = disp->d;

    if (shouldRedraw())
    {
        // don't have time for a full refresh when shutting down
        if(disp->getRefreshState() == DisplayRefreshState::Fresh){
            disp->DisableFullRefresh();
        }

        disp->StartRefresh();
        d->setRotation(2);
        d->fillScreen(white);
        d->drawBitmap(0, 0, bitmaps::power_off_screen, 200, 200, black);
        disp->EndRefresh();

        disp->EnableFullRefresh();
    }
}

void App::InvalidStateNotice(){
    auto d = disp->d;

    if(shouldRedraw()){
        
        disp->StartRefresh();
        d->setRotation(2);

        d->setFont(&FreeMonoBold9pt7b);
        d->setCursor(0, 96);
        d->print("The processor powered off incorrectly. Climate data has been lost and time may be incorrect");
        

        disp->EndRefresh();
    }
    if(getBtn2Down()){
        setState(State::StandardDisplay);
    }
}

void App::DebugMenu(){
    auto d = disp->d;

    if(shouldRedraw()){ 
        disp->StartRefresh();
        d->setRotation(1);
            
        d->fillScreen(white);
        d->setFont(&FreeMonoBold9pt7b);

        PowerStatus ps = {};
        power::GetPowerStatus(ps);

        d->drawCircle(4, 6, 2, black);
        disp->print(12, 10, "Back");
        d->drawCircle(104, 6, 2, black);
        disp->print(112, 10, "Refresh");
        d->drawLine(0, 14, 199, 14, black);

        {
            disp->printf(0, 40, "USB: %s", (int)power::USBConnected() ? "yes" : "no");
            disp->printf(100, 40, "CHG: %s", (int)ps.Charging ? "yes" : "no");
            disp->printf(0, 60, "voltage: %.3fV", ps.batteryVoltage);
            disp->printf(0, 80, "current: %dmA", (int)ps.averageCurrent_ma);
            disp->printf(0, 100, "SOC: %d%%", (int)ps.batteryCapacity_percentage);

            if(globalState->currentSettings.tempFormat == TemperatureFormat::Celcius)
                disp->printf(0, 120, "temp: %.2f C", globalState->lastClimateReading.Temperature_c());
            else
                disp->printf(0, 120, "temp: %.1f F", globalState->lastClimateReading.Temperature_f());
            
            disp->printf(0, 140, "humi: %.2f%%", globalState->lastClimateReading.Humidity_r());
            
            char timebuf[12];
            if(globalState->currentSettings.timeFormat == TimeFormat::_12Hour){
                GetTimeStringFull12(timebuf, currentTime);
                disp->printf(0, 160, "time: %s %s", timebuf, currentTime.Hour > 12 ? "pm" : "am");
            }
            else{
                GetTimeStringFull24(timebuf, currentTime);
                disp->printf(0, 160, "time: %s", timebuf);
            }

            char dwakeupBuf[32];
            if(globalState->lastDeepWakeupCause == DeepWakeupCause::Alarm){
                sprintf(dwakeupBuf, "alarm");
            }
            else if(globalState->lastDeepWakeupCause == DeepWakeupCause::ButtonFromHibernate){ 
                sprintf(dwakeupBuf, "p btn");
            }
            else if(globalState->lastDeepWakeupCause == DeepWakeupCause::FirmwareUpdate){ 
                sprintf(dwakeupBuf, "update");
            }
            else{
                sprintf(dwakeupBuf, "power rst");
            }

            char lwakeupBuf[32];
            if(globalState->lastLightWakeupCause == LightWakeupCause::MenuButton){
                sprintf(lwakeupBuf, "menu btn");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::ClockAlarm){
                sprintf(lwakeupBuf, "alarm");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::NA){
                sprintf(lwakeupBuf, "N/A");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::PowerButton){
                sprintf(lwakeupBuf, "p btn");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::Timer){
                sprintf(lwakeupBuf, "timer");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::Unknown){
                sprintf(lwakeupBuf, "unknown");
            }
            else if(globalState->lastLightWakeupCause == LightWakeupCause::Unknown_ext1){
                sprintf(lwakeupBuf, "? ext1");
            }
            

            disp->printf(0, 180, "Dwakeup: %s", dwakeupBuf);
            disp->printf(0, 199, "Lwakeup: %s", lwakeupBuf);
        }
        
        disp->EndRefresh();
    }

    if(getBtn2Down()){
        setState(State::MainMenu);
    }
    else if(getBtn1Down()){
        setRedraw();
    }
}


std::array<ClimateSample, flash::dataCapacity> climateHistory2;

void App::ViewData(){
        auto d = disp->d;

    if(shouldRedraw()){ 
        disp->StartRefresh();
        d->setRotation(1);
            
        d->fillScreen(white);
        d->setFont(&FreeMonoBold12pt7b);

        disp->print(0, 16, "Cool Graph");


        const int loadRange = 60 * 8; // get up to the last 8 hours
        uint16_t requestAmount = globalState->currentHeader.dataCount < loadRange ? globalState->currentHeader.dataCount : loadRange; 

        uint16_t actualCount;
        eeprom->GetClimateSamples(
            globalState->currentHeader, 
            climateHistory2.data(),  
            requestAmount,
            0,
            actualCount
        );
        uint16_t historyEnd = 0;
        uint16_t historyStart = actualCount;
        int readingCount = historyStart - historyEnd; // dumb?

        for (int i = historyEnd; i < historyStart; i++)
        {
            rawTemperatureBuffer[i - historyEnd] = climateHistory2[i].temperature_raw;
        }

        int min, max;
        drawGraph(0, 199, 13, 150, rawTemperatureBuffer.data(), readingCount, &min, &max);

        d->setFont(&FreeMonoBold9pt7b);
        if(readingCount > 1){
            disp->printf(0, 199, "T-%dm", (int(readingCount)));
            disp->printf(140, 199, "%.2f", min / 100.0f);
            disp->printf(140, 43, "%.2f", max / 100.0f);
        }
        else{
            disp->printf(0, 199, "No data", (int(readingCount)));
        }

        disp->EndRefresh();
    }

    if(getBtn2Down()){
        setState(State::MainMenu);
    }

    // never timeout while viewing data (this should be chnaged to a different, longer timeout instead of no timeout)
    resetInactivity();
}

void App::LowTempWarning(){
    
    auto d = disp->d;

    if(shouldRedraw()){ 

        disp->ForceFullNextRefresh();
        disp->StartRefresh();
        d->setRotation(2);
                
        d->fillScreen(white);
        d->setFont(&FreeMonoBold12pt7b);
        disp->print(0, 16, "Too Cold!");
        d->drawBitmap(151, 0, bitmaps::face_cold_1, 48, 48, black);
        d->setFont(&FreeMonoBold9pt7b);
        disp->printf(0, 32, "< %.1f C", Constants::lowTemperatureDisplayThreshold_C);

        int linh = 64;
        disp->print(0, linh, "It's too cold to");
        linh += 17;
        disp->print(0, linh, "update the screen.");
        linh += 17;
        disp->print(0, linh, "Climate is still");
        linh += 17;
        disp->print(0, linh, "being recorded.");
        linh += 17;
        disp->print(0, linh, "Display will");
        linh += 17;
        disp->print(0, linh, "reactivate when");
        linh += 17;
        disp->print(0, linh, "it warms up.");
        
        disp->EndRefresh();
    }
}

void App::TimeSet(){
    auto d = disp->d;

    

    if(timeSetStage == 0){
        timeSetStage = 1;

        timeSetHour = globalState->currentTime.Hour;
        timeSetMinute = globalState->currentTime.Minute;
    }

    if(shouldRedraw()){ 
        disp->StartRefresh();
        d->setRotation(1);
            
        d->fillScreen(white);
        d->setFont(&FreeMonoBold9pt7b);

        // top bar
        {
            d->drawCircle(14, 6, 2, black);
            disp->print(24, 10, "Next");
            
            d->drawCircle(4, 22, 2, black);
            d->drawCircle(14, 22, 2, black);
            disp->print(24, 26, "Cancel");


            d->drawCircle(170, 6, 2, black);
            disp->print(178, 10, "+1");

            d->drawCircle(160, 22, 2, black);
            d->drawCircle(170, 22, 2, black);
            disp->print(178, 26, "+5");
            
            d->drawFastHLine(0, 30, 199, black);
       }

        
        // time picker
        d->setFont(&FreeMonoBold24pt7b);
        
        char timebuf[12];
        if(globalState->currentSettings.timeFormat == TimeFormat::_12Hour){
            int hour12 = timeSetHour % 12;
            if (hour12 == 0) 
            hour12 = 12; // 0 or 12 becomes 12 in 12-hour time
            sprintf(timebuf, "%2d:%02d", hour12, timeSetMinute);
        }
        else{
            sprintf(timebuf, "%2d:%02d", timeSetHour, timeSetMinute);
        }
        
        int offset = disp->printAndOffset(10, 120, timebuf);
        if(timeSetStage == 1){
            d->drawFastHLine(15, 126, 40, black);
            d->drawBitmap(30, 128, bitmaps::up_arrow, 15, 24, black);
        }
        else if(timeSetStage == 2){
            d->drawFastHLine(100, 126, 40, black);
            d->drawBitmap(115, 128, bitmaps::up_arrow, 15, 24, black);
        }
        else if(timeSetStage == 3){
            d->drawBitmap(170, 128, bitmaps::up_arrow, 15, 24, black);
        }
        
        if(globalState->currentSettings.timeFormat == TimeFormat::_12Hour){
            d->setFont(&FreeMonoBold18pt7b);
            if(timeSetHour < 12)
                disp->print(offset + 7, 118, "am");
            else
                disp->print(offset + 7, 118, "pm");
        }

        disp->EndRefresh();
    }

    if(getBtn2MultiPressConfirmed() == 1){
        setRedraw();
        timeSetStage ++;
        if(
            (globalState->currentSettings.timeFormat == TimeFormat::_12Hour && timeSetStage >= 4) ||
            (globalState->currentSettings.timeFormat == TimeFormat::_24Hour && timeSetStage >= 3)){
            // set the time and exit
            DateTime dt;
            dt.Hour = static_cast<uint8_t>(timeSetHour);
            dt.Minute = static_cast<uint8_t>(timeSetMinute);
            dt.Second = 0;
            clock->Write(dt); 
            globalState->currentTime.Hour = dt.Hour;
            globalState->currentTime.Minute = dt.Minute;
            globalState->currentTime.Second = dt.Second;

            globalState->shouldGoToSleep = true;
            setState(State::StandardDisplay);
        }
    }
    else if(getBtn2MultiPressimmediate() == 2){
        timeSetStage = 0;
        setState(State::MainMenu);
    }
    else if(getBtn1MultiPressConfirmed() == 1){
        if(timeSetStage == 1){
            timeSetHour++;
            timeSetHour %= 24;
            setRedraw();
        }
        else if(timeSetStage == 2){
            timeSetMinute++;
            timeSetMinute %= 60;
            setRedraw();
        }
        else if(timeSetStage == 3){
            timeSetHour += 12;
            timeSetHour %= 24;
            setRedraw();
        }
    }
    else if(getBtn1Down() && getBtn1MultiPressimmediate() == 2){
        if(timeSetStage == 1){
            if(timeSetHour > 18)
                timeSetHour = 0;
            else
                timeSetHour += 5;
            setRedraw();
        }
        else if(timeSetStage == 2){
            timeSetMinute += 5;
            if(timeSetMinute > 59)
                timeSetMinute = 0;
            setRedraw();
        }
    }
    

}

void App::ChargingScreen(){
    
    auto d = disp->d;

    if(shouldRedraw()){ 

        disp->ForceFullNextRefresh();

        disp->StartRefresh();
        d->setRotation(2);
            
        d->fillScreen(white);

        PowerStatus ps = {};
        power::GetPowerStatus(ps);

        d->drawBitmap(15, 20, bitmaps::charging_battery, 175, 77, black);

        d->setFont(&FreeMonoBold12pt7b);
        disp->print(10, 145, "Charging");
        d->drawBitmap(144, 120, bitmaps::face_happy_1, 48, 48, black);

        disp->EndRefresh();

        disp->Hibernate();
    }

    if(getBtn2MultiPressimmediate() == 2){
        setState(State::MainMenu);
    }
}

void App::SettingsMenu(){
 auto d = disp->d;

    if(shouldRedraw()){ 
        disp->StartRefresh();
        d->setRotation(1);
            
        d->fillScreen(white);
        d->setFont(&FreeMonoBold9pt7b);

        // top bar
        {
            d->drawCircle(14, 6, 2, black);
            disp->print(24, 10, "Next");
            
            d->drawCircle(4, 22, 2, black);
            d->drawCircle(14, 22, 2, black);
            disp->print(24, 26, "Back");


            d->drawCircle(120, 6, 2, black);
            disp->print(128, 10, "Change");
            
            d->drawFastHLine(0, 30, 199, black);
        }

        disp->print(0, 50, "Temp unit:");
        disp->printf(163, 50, "%c", globalState->currentSettings.tempFormat == TemperatureFormat::Celcius ? 'C' : 'F');

        disp->print(0, 70, "Time format:");
        disp->printf(160, 70, "%s", globalState->currentSettings.timeFormat == TimeFormat::_12Hour ? "12h" : "24h");

        disp->print(0, 90, "Graph range:");
        switch (globalState->currentSettings.graphRange)
        {
            case GraphRange::_10Minutes:
                disp->print(160, 90, "10m");
                break;
            case GraphRange::_20Minutes:
                disp->print(160, 90, "20m");
                break;
            case GraphRange::_30Minutes:
                disp->print(160, 90, "30m");
                break;
            case GraphRange::_60Minutes:
                disp->print(160, 90, "60m");
                break;
            default:
                break;
        }

        d->drawRect(140, 35 + selectedSetting * 20, 58, 20, black);

        disp->EndRefresh();
    }

    if(getBtn1Down()){

        if (selectedSetting == 0)
        {
            if (globalState->currentSettings.tempFormat == TemperatureFormat::Celcius)
                globalState->currentSettings.tempFormat = TemperatureFormat::Fahrenheit;
            else
                globalState->currentSettings.tempFormat = TemperatureFormat::Celcius;
        }
        else if (selectedSetting == 1)
        {
            if (globalState->currentSettings.timeFormat == TimeFormat::_12Hour)
                globalState->currentSettings.timeFormat = TimeFormat::_24Hour;
            else
                globalState->currentSettings.timeFormat = TimeFormat::_12Hour;
        }
        else if(selectedSetting == 2)
        {
            switch (globalState->currentSettings.graphRange)
            {
            case GraphRange::_10Minutes:
                globalState->currentSettings.graphRange = GraphRange::_20Minutes;
                break;
            case GraphRange::_20Minutes:
                globalState->currentSettings.graphRange = GraphRange::_30Minutes;
                break;
            case GraphRange::_30Minutes:
                globalState->currentSettings.graphRange = GraphRange::_60Minutes;
                break;
            case GraphRange::_60Minutes:
                globalState->currentSettings.graphRange = GraphRange::_10Minutes;
                break;
            default:
                break;
            }
        }

        eeprom->WriteSettings(globalState->currentSettings);

        setRedraw();
    }
    else if(getBtn2MultiPressConfirmed() == 1){
        selectedSetting++;
        selectedSetting %= 3;
        setRedraw();
    }
    else if(getBtn2MultiPressimmediate() == 2){
        timeSetStage = 0;
        setState(State::MainMenu);
    }
}

void App::WelcomeScreen(){
    auto d = disp->d;

    if(shouldRedraw()){ 
        disp->StartRefresh();
        d->setRotation(2);
            
        d->fillScreen(white);
        d->drawBitmap(0, 0, bitmaps::welcom_screen, 200, 200, black);
        disp->EndRefresh();
    }

    if(getBtn1Down() || getBtn2Down()){
        setState(State::StandardDisplay);
        globalState->shouldGoToSleep = true;
    }
}