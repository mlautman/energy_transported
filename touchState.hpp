#ifndef TOUCHSTATE_H
#define TOUCHSTATE_H
#include "util.hpp"
#include <string> 


// #define TOUCH_THRESHOLD 10
#define HISTORY_LEN 3

enum TouchStateID {
    NO_TOUCH=0, LEFT_ONLY, RIGHT_ONLY, BOTH_NO_CON, BOTH_CON
};

std::string touchToString(TouchStateID);

class TouchState {
public:
    TouchState(uint touch_pin_0, uint touch_pin_1, uint input_pin, uint output_pin);
    void calibrateCapValues();
    bool update();
    TouchStateID getState() const;
    std::string getStateString();
    double getLeftCapValue() const;
    double getRightCapValue() const;
    uint getLeftCapValueRaw() const;
    uint getRightCapValueRaw() const;
    void printFullState();
private:
    uint touch_pin_0;
    uint touch_pin_1;
    uint input_pin;
    uint output_pin;
    
    uint state_history[HISTORY_LEN] = {0}; // Correct array declaration

    uint leftCapCalibrationValue;
    uint rightCapCalibrationValue;

    uint connected;
    uint leftCapValue;
    uint rightCapValue;
    TouchStateID state = NO_TOUCH;
    void updateCapValues();
    void printCapValues();
    bool updateState();
    TouchStateID getFilteredState(TouchStateID new_state);
    void updateConnected();
};

#endif