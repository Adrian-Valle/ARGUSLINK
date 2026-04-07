#include <SPI.h>
#include <LoRa.h>
#include "config.h"
#include "pins.h"
#include "lora_radio.h"

void initLoRa() {
 LoRa.setPins(PIN_LORA_NSS, PIN_LORA_RST, PIN_LORA_DIO0);

 if (!LoRa.begin(LORA_FREQUENCY)) {
   Serial.println("Error: Fallo al iniciar LoRa");
   while (true);
 }

 LoRa.setSignalBandwidth(LORA_BANDWIDTH);
 LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);

 Serial.println("LoRa configurado correctamente");
}

void sendLoRaMessage(const String &message) {
 LoRa.beginPacket();
 LoRa.print(message);
 LoRa.endPacket();

 Serial.print("Enviado por LoRa: ");
 Serial.println(message);
}
