#include "config.h"
#include "pins.h"
#include "button.h"
#include "battery.h"
#include "comms.h"
#include "state_machine.h"

void setup() {
  initButton();
  initBattery();
  initComms();
  initStateMachine();
}

void loop() {
  updateButton();
  updateBattery();
  updateStateMachine();
}