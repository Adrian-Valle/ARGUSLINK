#include <SPI.h>
#include <LoRa.h>

const int csPin = 10;
const int resetPin = 9;
const int irqPin = 2;

void setup() {
 Serial.begin(9600);
 while (!Serial);

 Serial.println("--- RECEPTOR ARGUSLINK ---");

 LoRa.setPins(csPin, resetPin, irqPin);

 if (!LoRa.begin(868E6)) {
   Serial.println("Error: No se encuentra el módulo LoRa receptor.");
   while (true);
 }

 LoRa.setSignalBandwidth(125E3);
 LoRa.setSpreadingFactor(7);

 Serial.println("Receptor listo y esperando mensajes...");
}

void loop() {
 int packetSize = LoRa.parsePacket();

 if (packetSize) {
   Serial.print("Mensaje recibido: ");

   String mensaje = "";
   while (LoRa.available()) {
     mensaje += (char)LoRa.read();
   }

   Serial.print(mensaje);
   Serial.print(" | RSSI: ");
   Serial.println(LoRa.packetRssi());
 }
}