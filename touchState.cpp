#include "touchState.hpp"
#include "Arduino.h"



std::string touchToString(TouchStateID id) {
  switch (id) {
    case NO_TOUCH:
      return "NO_TOUCH";
    case LEFT_ONLY:
      return "LEFT_ONLY";
    case RIGHT_ONLY:
      return "RIGHT_ONLY";
    case BOTH_NO_CON:
      return "BOTH_NO_CON";
    case BOTH_CON:
      return "BOTH_CON";
    default:
      return "UNKNOWN";
  }
}

TouchState::TouchState(uint touch_pin_0, uint touch_pin_1, uint input_pin, uint output_pin):
    touch_pin_0(touch_pin_0), touch_pin_1(touch_pin_1), input_pin(input_pin), output_pin(output_pin)
{
    touchAttachInterrupt(touch_pin_0, nullptr, 0); // Dummy interrupt to activate touch on GPIO4
    touchAttachInterrupt(touch_pin_1, nullptr, 0); // Dummy interrupt to activate touch on GPIO15
    // we set the output pin and input to input but without
    // a pullup/down resistor so that it minimizes effect the
    // capacitive touch sensors.
    pinMode(output_pin, INPUT);
    pinMode(input_pin, INPUT);
    calibrateCapValues();
}

void TouchState::calibrateCapValues()
{
  // we set the output pin and input to input but without
  // a pullup/down resistor so that it minimizes effect the
  // capacitive touch sensors.
  pinMode(output_pin, INPUT);
  pinMode(input_pin, INPUT);
  // settle
  uint calibration_runs = 10;
  leftCapCalibrationValue = 2;
  rightCapCalibrationValue = 2;
  for (uint i = 0; i < calibration_runs; i++) {
    nbDelay(50);
    leftCapCalibrationValue += touchRead(touch_pin_0);
    rightCapCalibrationValue += touchRead(touch_pin_1);
  }
  leftCapCalibrationValue = leftCapCalibrationValue / calibration_runs;
  rightCapCalibrationValue = rightCapCalibrationValue / calibration_runs;
}

bool TouchState::update() 
{
  updateCapValues();
  return updateState();
}

TouchStateID TouchState::getState() const
{
  return state;
}

std::string TouchState::getStateString() 
{
  return touchToString(state);
}

double TouchState::getLeftCapValue() const
{
  return (double)(leftCapCalibrationValue - leftCapValue) / leftCapCalibrationValue;  
}

double TouchState::getRightCapValue() const
{
  return (double)(rightCapCalibrationValue-rightCapValue) / rightCapCalibrationValue;  
}

uint TouchState::getLeftCapValueRaw() const
{
  return leftCapValue;
}

uint TouchState::getRightCapValueRaw() const
{
    return rightCapValue;
}

void TouchState::printFullState() 
{
  Serial.print("Left: \t");
  Serial.print(leftCapValue);
  Serial.print("\t Right: \t");
  Serial.print(rightCapValue);
  Serial.print("\t Left Cal: \t");
  Serial.print(leftCapCalibrationValue);
  Serial.print("\t Right Cal: \t");
  Serial.print(rightCapCalibrationValue);
  Serial.print("\t Connected: \t");
  Serial.print(connected);
  Serial.print("\t State: \t");
  Serial.println(touchToString(state).c_str());

}

void TouchState::updateCapValues()
{
    // we set the output pin and input to input but without
    // a pullup/down resistor so that it minimizes effect the
    // capacitive touch sensors.
    pinMode(output_pin, INPUT);
    pinMode(input_pin, INPUT);
    // settle
    nbDelay(1);
    leftCapValue = touchRead(touch_pin_0);
    rightCapValue = touchRead(touch_pin_1);
}

void TouchState::printCapValues() 
{
    Serial.print("Left: \t");
    Serial.print(leftCapValue);
    Serial.print("\t Right: \t");
    Serial.println(rightCapValue);
}


void TouchState::updateConnected()
{
  //  We tried this to disable the hardware touch sensing on those pins but unfortunately it didn't work
  pinMode(input_pin, INPUT);
  pinMode(output_pin, INPUT);
  //  Allow time for pins to settle
  nbDelay(2);

  // Set the IO pins
  pinMode(output_pin, OUTPUT);
  pinMode(input_pin, INPUT_PULLDOWN);
  //  Settle
  nbDelay(2);

  // Set output to high
  digitalWrite(output_pin, HIGH);

  // Settle
  nbDelay(2);
  // Read the input
  connected = (TouchStateID)digitalRead(input_pin);
  // Set the output pin to low
  digitalWrite(output_pin, LOW);
  nbDelay(1);
  // Settle

  // we set the output pin and input to input but without
  // a pullup/down resistor so that it does not affect the
  // capacitive touch sensors.
  pinMode(output_pin, INPUT);
  pinMode(input_pin, INPUT);
  nbDelay(1);

  touchAttachInterrupt(touch_pin_0, nullptr, 0); // Dummy interrupt to activate touch on GPIO4
  touchAttachInterrupt(touch_pin_1, nullptr, 0); // Dummy interrupt to activate touch on GPIO15
  nbDelay(1);

  // Settle
  nbDelay(10);
}

TouchStateID TouchState::getFilteredState(TouchStateID new_state)
{
  // update the history
  for (uint i = 1; i < HISTORY_LEN; i++) {
      state_history[i-1] = state_history[i];
  }
  state_history[HISTORY_LEN - 1] = new_state;
  
  bool all_same = true;

  for (int i = 0; i < HISTORY_LEN; i++) {
    if (state_history[i] != new_state) {
      all_same = false;
      break;
    }
  }

  if (all_same) {
    state = new_state;
  }    
  
  return state;
}

bool TouchState::updateState()
{
  updateCapValues();
  updateConnected();
  TouchStateID measured_state = NO_TOUCH;
  if (leftCapValue < leftCapCalibrationValue && rightCapValue < rightCapCalibrationValue) {
    if (connected) {
      measured_state = BOTH_CON;
    } else {
      measured_state = BOTH_NO_CON;
    }
  } else if (leftCapValue < leftCapCalibrationValue) {
    measured_state = LEFT_ONLY;
  } else if (rightCapValue < rightCapCalibrationValue) {
    measured_state = RIGHT_ONLY;
  } else {
    measured_state = NO_TOUCH;
  }
  TouchStateID previous_state = state;
  TouchStateID new_state = getFilteredState(measured_state);
  return previous_state != new_state;
}
