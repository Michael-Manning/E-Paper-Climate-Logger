#include <stdint.h>

#include <buttons.h>


bool prevBtn1State = false;
bool prevBtn2State = false;
bool currentBtn1State = false;
bool currentBtn2State = false;

uint32_t currentTimestampMS = 0;

uint32_t btn1LastDownMS = 0;
uint32_t btn1HoldStartMS = 0;
int btn1MultipressCount = 0;
int btn1MultipressCountConfirmed = 0;
bool btn1MultipressLatched = false;

uint32_t btn2LastDownMS = 0;
uint32_t btn2HoldStartMS = 0;
int btn2MultipressCount = 0;
int btn2MultipressCountConfirmed = 0;
bool btn2MultipressLatched = false;


/////////////////////// Button 1 functions ////////////////////////

bool getBtn1(){
    return currentBtn1State;
}

bool getBtn1Down(){
    return currentBtn1State && !prevBtn1State;
}
bool getBtn1Up(){
    return !currentBtn1State && prevBtn1State;
}

uint32_t getBtn1HeldMS(){
    return currentTimestampMS - btn1HoldStartMS;
}

int getBtn1MultiPressimmediate(){
    return btn1MultipressCount;
}

int getBtn1MultiPressConfirmed(){
    if(btn1MultipressCount == maxMultipress)
        return maxMultipress;
    if(btn1MultipressCountConfirmed == -1)
        return 0;
    return btn1MultipressCountConfirmed;
}


/////////////////////// Button 2 functions ////////////////////////

bool getBtn2(){
    return currentBtn2State;
}

bool getBtn2Down(){
    return currentBtn2State && !prevBtn2State;
}
bool getBtn2Up(){
    return !currentBtn2State && prevBtn2State;
}

uint32_t getBtn2HeldMS(){
    return currentTimestampMS - btn2HoldStartMS;
}

int getBtn2MultiPressimmediate(){
    return btn2MultipressCount;
}

int getBtn2MultiPressConfirmed(){
    if(btn2MultipressCount == maxMultipress)
        return maxMultipress;
    if(btn2MultipressCountConfirmed == -1)
        return 0;
    return btn2MultipressCountConfirmed;
}


void updateButtonState(bool btn1State, bool btn2State, uint32_t timestampMS){
    prevBtn1State = currentBtn1State;
    prevBtn2State = currentBtn2State;
    currentBtn1State = btn1State;
    currentBtn2State = btn2State;
    currentTimestampMS = timestampMS;

    btn1MultipressCountConfirmed = -1;
    btn2MultipressCountConfirmed = -1;

    // I don't know how to comment this function. It's just raw state logic

    if(getBtn1Down())
    {
        if(currentTimestampMS - btn1LastDownMS < multipressMaxPeriodMS)
        {
            if(btn1MultipressCount < maxMultipress && btn1MultipressCount != 0)
                btn1MultipressCount++;   
        }
        else if (btn1MultipressCount == 0)
        {
            btn1MultipressCount++;
        }

        btn1LastDownMS = timestampMS;
    }
    else{
        if(btn1MultipressCount == maxMultipress)
            btn1MultipressCount = 0;
        

        if(currentTimestampMS - btn1LastDownMS >= multipressMaxPeriodMS && btn1MultipressCount > 0)
        {
            btn1MultipressCountConfirmed = btn1MultipressCount;
            btn1MultipressCount = 0;
        }
    }
    if(!getBtn1())
        btn1HoldStartMS = timestampMS;
    
    
    if(getBtn2Down())
    {
        if(currentTimestampMS - btn2LastDownMS < multipressMaxPeriodMS)
        {
            if(btn2MultipressCount < maxMultipress && btn2MultipressCount != 0)
                btn2MultipressCount++;   
        }
        else if (btn2MultipressCount == 0)
        {
            btn2MultipressCount++;
        }

        btn2LastDownMS = timestampMS;
    }
    else{
        if(btn2MultipressCount == maxMultipress)
            btn2MultipressCount = 0;
        

        if(currentTimestampMS - btn2LastDownMS >= multipressMaxPeriodMS && btn2MultipressCount > 0)
        {
            btn2MultipressCountConfirmed = btn2MultipressCount;
            btn2MultipressCount = 0;
        }
    }
    if(!getBtn2())
        btn2HoldStartMS = timestampMS;
}



bool debounceButton1(bool rawState, uint32_t timestampMS, bool force)
{
    constexpr float     alpha         = 0.20f;   // EWMA weight
    constexpr float     threshHigh    = 0.80f;   // Rising threshold
    constexpr float     threshLow     = 0.20f;   // Falling threshold

    static float        filt       = rawState ? 1.0f : 0.0f; // EWMA accumulator
    static bool         state      = rawState;               // Last accepted state
    static uint32_t     lastChange = 0;                      // Time of last change

    // Bypass filtering and hysteresis if requested.
    if (force)
    {
        state      = rawState;                 // accept immediately
        filt       = rawState ? 1.0f : 0.0f;   // saturate the filter
        lastChange = timestampMS - minTransitionIntervalMS; // treat as stable "for a long time"
        return state;
    }

    // Normal EWMA update
    filt = alpha * static_cast<float>(rawState) + (1.0f - alpha) * filt;

    // Hysteresis to get candidate logic level
    bool candidate = state;
    if (filt >= threshHigh)      candidate = true;
    else if (filt <= threshLow)  candidate = false;

    // Accept transition only if lock-out time has elapsed
    if (candidate != state && (timestampMS - lastChange) >= minTransitionIntervalMS)
    {
        state      = candidate;
        lastChange = timestampMS;
    }
    return state;
}