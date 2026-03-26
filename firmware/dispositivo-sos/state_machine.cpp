#include "state_machine.h"
#include "button.h"
#include "comms.h"

void initStateMachine() {
}

void updateStateMachine() {
  if (isButtonPressed()) {
    sendSOSMessage();
  }
}