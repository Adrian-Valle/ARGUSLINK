#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RadioLib.h>

// --- CONFIGURACIÓN LORA ---
// NSS: 10, DIO0: 4, RST: 9, DIO1: 3
SX1276 radio = new Module(10, 4, 9, 3);

const float FRECUENCIA = 868.0; 
const byte SYNC_WORD = 0x12;    

// --- CONFIGURACIÓN SENSORES ---
const char LOCATION[] = "Sala principal";
const unsigned long INTERVALO_MS = 10000UL; 
const int PIN_MQ135 = A0;

Adafruit_BME280 bme;
unsigned long ultimoEnvio = 0;
bool bmeOk = false;

void setup() {
  Serial.begin(115200);
  while (!Serial); 
  
  Serial.println(F("\n--- INICIANDO EMISOR ---"));

  // 1. Inicializar BME280 con reintento
  Wire.begin();
  if (bme.begin(0x76) || bme.begin(0x77)) {
    Serial.println(F("[Sensor] BME280 detectado."));
    bmeOk = true;
  } else {
    Serial.println(F("[Sensor] ERROR: No se encuentra BME280."));
  }
  
  pinMode(PIN_MQ135, INPUT);

  // 2. Inicializar LoRa
  Serial.print(F("[LoRa] Configurando chip... "));
  int state = radio.begin(FRECUENCIA);
  
  if (state == RADIOLIB_ERR_NONE) {
    radio.setSyncWord(SYNC_WORD);
    radio.setOutputPower(17); 
    Serial.println(F("OK!"));
  } else {
    Serial.print(F("Fallo crítico LoRa, código: "));
    Serial.println(state);
    while (true); 
  }
}

void loop() {
  if (millis() - ultimoEnvio < INTERVALO_MS) return;
  ultimoEnvio = millis();

  Serial.println(F("\n--- Ciclo de Lectura ---"));
  
  float temp = 0, pres = 0, hum = 0;
  if (bmeOk) {
    temp = bme.readTemperature();
    pres = bme.readPressure() / 100.0F;
    hum = bme.readHumidity();
    
    // Validación de datos reales
    if (temp > 80 || temp < -20) { 
      Serial.println(F("[!] Lectura BME280 inválida, reintentando sensor..."));
      bme.begin(0x76); // Intento de reinicio suave
      temp = 0; pres = 0; hum = 0;
    }
  }
  
  int gas = analogRead(PIN_MQ135);

  // Crear mensaje: Loc|Temp|Pres|Hum|Gas
  String mensaje = String(LOCATION) + "|" + 
                   String(temp, 1) + "|" + 
                   String(pres, 1) + "|" + 
                   String(hum, 1) + "|" + 
                   String(gas);

  Serial.print(F("[LoRa] Transmitiendo: "));
  Serial.println(mensaje);

  // Intentar envío
  int state = radio.transmit(mensaje);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[EXITO] Paquete enviado correctamente."));
    // Hemos eliminado la línea de getDataRate que daba error
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[ERROR] Timeout (Error -5). Revisa el pin DIO0 (Pin 4)."));
  } else {
    Serial.print(F("[ERROR] Fallo código: "));
    Serial.println(state);
  }
}