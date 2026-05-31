

#include <SPI.h>

#include <stdio.h>

#include <Arduino.h>

#include "MAssert.h"
#include "debugLog.h"
#include "display.h"
#include "EEPROM.h"
#include "Constants.h"
#include "pinout.h"

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#define USE_HSPI_FOR_EPD
SPIClass hspi(HSPI);


GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(pins::dsp_CS, pins::dsp_DC, pins::dsp_RST, pins::dsp_BUSY)); // GDEH0154D67


void centerPrint(const char* str, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t cx = x + ((w - tbw) / 2) - tbx;
  uint16_t cy = y + ((h - tbh) / 2) - tby;
  display.setCursor(cx, cy);
  display.print(str);
}


// This display has extremely weird refresh behaviour that I don't fully understand. I merely work around what is repeatable.
// The display's ghosting and update quality depends on what was previously displayed, as well as what type of refresh was
// previously performed to display it.
//
// Here is what I understand in order to get reliable quality while minimizing refreshes and have implemented:
// - when starting up, the first update must be a full refresh
// - the next update after a full refresh may be partial, but the contents must be significantly different, or a full white screen, then an additional partial refresh of desired content
// - the next n updates may be partial refreshes with any content with only mild artifact buildup that must be occasionally cleared by restarting this sequence.


void Display::StartRefresh(){
    MASSERT(refreshInprogress == false);

    if(hibernating || shutDown){
        Reset();
    }

    if(getPartialRefreshCount() >= Constants::maxPartialFreshres && fullRefreshEnabled){
        ForceFullNextRefresh();
    }

    if(getRefreshState() == DisplayRefreshState::Fresh){
        doingFullRefresh = true;
        setRefreshState(DisplayRefreshState::FullRefresh);
        resetPartialRefreshCount();
    }
    else if(getRefreshState() == DisplayRefreshState::FullRefresh){
        // perform immediate partial blank to white
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.display(true);

        doingFullRefresh = false;
        setRefreshState(DisplayRefreshState::PartialFullBlank);
    }
    else if(getRefreshState() == DisplayRefreshState::PartialFullBlank){
        doingFullRefresh = false;
        setRefreshState(DisplayRefreshState::Partial);
    }

    refreshInprogress = true;
}

void Display::EndRefresh(){
    MASSERT(refreshInprogress == true);
    display.display(!doingFullRefresh);
    refreshInprogress = false;
    incrementPartialRefreshCount();
}

void Display::Reset(){
   display.init(115200, shutDown, 2, false);
   display.setRotation(1);
   hibernating = false;
}

void Display::FullClear(){
    ForceFullNextRefresh();
    StartRefresh();
    display.fillScreen(GxEPD_WHITE);
    EndRefresh();
}

void Display::Hibernate(){
    if(!hibernating){
        display.hibernate();
        hibernating = true;
    }
}

void Display::ShutDown(){
    shutDown = true;
    ForceFullNextRefresh();
    display.powerOff();
}

void Display::ForceFullNextRefresh(){
    setRefreshState(DisplayRefreshState::Fresh);
}

void Display::EnableFullRefresh(){
    fullRefreshEnabled = true;
}
void Display::DisableFullRefresh(){
    fullRefreshEnabled = false;
}

void Display::print(int x, int y, const char* str){
    display.setCursor(x, y);
    display.print(str);
}
int Display::printAndOffset(int x, int y, const char* str){
    display.setCursor(x, y);

    int16_t x1,y1;
    uint16_t w,h;
    d->getTextBounds(str, x, y, &x1, &y1, &w, &h);
    display.print(str);
    return x1 + w;
}

char printBuffer[128];
void Display::printf(int x, int y, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(printBuffer, fmt, args);
    va_end(args);

    display.setCursor(x, y);
    display.print(printBuffer);
}
int Display::printfAndOffset(int x, int y, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(printBuffer, fmt, args);
    va_end(args);

    display.setCursor(x, y);
    int16_t x1,y1;
    uint16_t w,h;
    d->getTextBounds(printBuffer, x, y, &x1, &y1, &w, &h);
    display.print(printBuffer);
    return x1 + w;
}


void Display::Init(bool fresh, StatusHeader* currentStatusHeader){
    d = &display;
    this->_currentStatusHeader = currentStatusHeader;

    hspi.begin(pins::dsp_CLK, -1, pins::dsp_SDI, pins::dsp_CS); // remap hspi for EPD (swap pins)
    display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));

    display.init(115200, fresh, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
    shutDown = false;
    hibernating = false;

    display.setRotation(1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    if(fresh)
        setRefreshState(DisplayRefreshState::Fresh);
}

uint8_t Display::getPartialRefreshCount(){
    return _currentStatusHeader->partialRefreshCount;
}
void Display::incrementPartialRefreshCount(){
    if(_currentStatusHeader->partialRefreshCount < 255)
        _currentStatusHeader->partialRefreshCount++;
}
void Display::resetPartialRefreshCount(){
    _currentStatusHeader->partialRefreshCount = 0;
}
DisplayRefreshState Display::getRefreshState(){
    return _currentStatusHeader->refreshState;
}
void Display::setRefreshState(DisplayRefreshState refreshState){
    _currentStatusHeader->refreshState = refreshState;
}