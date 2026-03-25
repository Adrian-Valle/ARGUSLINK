#include "battery.h"
#include "pins.h"

void initBattery() {
  pinMode(PIN_BATTERY, INPUT);
}

void updateBattery() {
}

float getBatteryLevel() {
  return analogRead(PIN_BATTERY);
}