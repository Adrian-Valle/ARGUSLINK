#include "button.h"
#include "pins.h"

void initButton() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void updateButton() {
}

bool isButtonPressed() {
  return digitalRead(PIN_BUTTON) == LOW;
}