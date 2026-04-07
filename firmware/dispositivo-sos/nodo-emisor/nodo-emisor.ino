#include <Arduino.h>

#include "config.h"
#include "pins.h"
#include "lora_radio.h"
#include "wifi_portal.h"

void setup() {
 Serial.begin(115200);
 while (!Serial);

 initLoRa();
 initWiFiPortal();

 Serial.println("ARGUSLINK: Sistema inicializado");
}

void loop() {
 handlePortal();

 if (hasPortalMessage()) {
   String msg = consumePortalMessage();
   sendLoRaMessage(msg);
 }
}
