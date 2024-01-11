#include "util.hpp"
#include "Arduino.h"

void nbDelay(unsigned long interval) {
  unsigned long initial_milli = millis();
  while (millis() - initial_milli < interval) {
    // Loop until the interval has passed
  }
  return;
}
