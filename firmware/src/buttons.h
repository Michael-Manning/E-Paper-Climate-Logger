#pragma once

#include <stdint.h>

constexpr uint8_t maxMultipress = 4;
constexpr uint16_t multipressMaxPeriodMS = 300;
constexpr uint32_t minTransitionIntervalMS = 30; 

bool getBtn1();
bool getBtn2();

bool getBtn1Down();
bool getBtn2Down();

bool getBtn1Up();
bool getBtn2Up();

uint32_t getBtn1HeldMS();
uint32_t getBtn2HeldMS();


int getBtn1MultiPressimmediate();
int getBtn2MultiPressimmediate();

int getBtn1MultiPressConfirmed();
int getBtn2MultiPressConfirmed();


void updateButtonState(bool btn1, bool btn2, uint32_t timestampMS);

bool debounceButton1(bool rawState, uint32_t timestampMS, bool force);