#pragma once

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "EEPROM.h"

// This class only exists to strip protected access of the Adafruit GFX charBounds function.
class DisplayGFXSubclass : public GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>
{
    public:
        DisplayGFXSubclass(int16_t cs, int16_t dc, int16_t rst, int16_t busy)
            : GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>(GxEPD2_154_D67(cs, dc, rst, busy)) // GDEH0154D67
        {
        }

    void calculateCharBounds(
        unsigned char c,
        int16_t *x,
        int16_t *y,
        int16_t *minX,
        int16_t *minY,
        int16_t *maxX,
        int16_t *maxY
    ) {
        charBounds(c, x, y, minX, minY, maxX, maxY);
    }
};

class Display{
public:
    void Init(bool fresh, StatusHeader* currentStatusHeader);
    void Reset();
    void StartRefresh();
    void EndRefresh();
    void FullClear();
    void Hibernate();
    void ShutDown();
    void ForceFullNextRefresh();
    void EnableFullRefresh();
    void DisableFullRefresh();

    void print(int x, int y, const char* str);
    int  printAndOffset(int x, int y, const char* str);
    void printf(int x, int y, const char* fmt, ...);
    int  printfAndOffset(int x, int y, const char* fmt, ...);

    DisplayGFXSubclass *d;

    DisplayRefreshState getRefreshState();
    uint8_t getPartialRefreshCount();
    
private:

    // simpler to just directly wire the status header than passing get/set callback function pointers
    StatusHeader* _currentStatusHeader = nullptr;

    void incrementPartialRefreshCount();
    void resetPartialRefreshCount();
    void setRefreshState(DisplayRefreshState refreshState);

    bool fullRefreshEnabled = true;
    uint32_t hibernateTimer = 0;
    bool hibernating = false;
    bool shutDown = true;
    bool refreshInprogress = false;
    bool doingFullRefresh = false;
};