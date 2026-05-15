#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <RadioLib.h>

// ============================================================
// CONFIGURACIÓN
// ============================================================
const char WIFI_SSID[]  = "Mr.Robot";
const char WIFI_PASS[]  = "Cw4cL.Ikyd";

// NSS: 10, DIO0: 4, RST: 9, DIO1: 3
SX1276 radio = new Module(10, 4, 9, 3);
const float FRECUENCIA = 868.0;
const byte SYNC_WORD = 0x12;

const char FB_HOST[]    = "arguslink-default-rtdb.europe-west1.firebasedatabase.app";
const char FB_API_KEY[] = "AIzaSyCaFxyDqldwG48IqOrjmZjqxH6THaFqA98";
const char FB_EMAIL[]   = "arguslink10@gmail.com";
const char FB_PASSWORD[]= "avelino2000";

String fbIdToken = "";
unsigned long tokenExpiry = 0;

WiFiSSLClient sslClient;

// ---------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------
static String authField(const String& json, const String& key) {
  String needle = "\"" + key + "\":";
  int i = json.indexOf(needle);
  if (i < 0) return "";
  i += needle.length();
  while (i < (int)json.length() && json[i] == ' ') i++;
  if (json[i] != '"') return "";
  i++;
  int j = json.indexOf('"', i);
  return (j < 0) ? "" : json.substring(i, j);
}

// Autenticación con reintentos
bool signIn() {
  for (int intento = 1; intento <= 3; intento++) {
    Serial.print(F("[Auth] Intento ")); Serial.print(intento); Serial.print(F("... "));
    
    sslClient.stop(); // Limpiar socket previo
    HttpClient http(sslClient, "identitytoolkit.googleapis.com", 443);
    http.setHttpResponseTimeout(8000); // Aumentamos a 8 seg para el handshake SSL

    String path = "/v1/accounts:signInWithPassword?key=" + String(FB_API_KEY);
    String body = "{\"email\":\"" + String(FB_EMAIL) + "\",\"password\":\"" + String(FB_PASSWORD) + "\",\"returnSecureToken\":true}";
    
    http.post(path, "application/json", body);
    int status = http.responseStatusCode();
    String resp = http.responseBody();
    
    if (status == 200) {
      fbIdToken = authField(resp, "idToken");
      tokenExpiry = millis() + (authField(resp, "expiresIn").toInt() - 60) * 1000UL;
      Serial.println(F("OK"));
      return true;
    }
    Serial.print(F("Error ")); Serial.println(status);
    delay(2000); // Esperar antes de reintentar
  }
  return false;
}

bool conectarSTA() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.print(F("Conectando WiFi..."));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" ¡OK!"));
    return true;
  }
  Serial.println(F(" FALLO"));
  return false;
}

bool enviarAFirebase(const String& sid, float val, const String& loc) {
  HttpClient fb(sslClient, FB_HOST, 443);
  fb.setHttpResponseTimeout(5000);

  String path = "/readings/" + sid + ".json?auth=" + fbIdToken;
  String body = "{\"value\":" + String(val, 2) + ",\"ts\":{\".sv\":\"timestamp\"},\"location\":\""+loc+"\"}";

  Serial.print(F("Subiendo ")); Serial.print(sid); Serial.print(F("... "));
  fb.post(path, "application/json", body);
  int status = fb.responseStatusCode();
  fb.responseBody(); 

  if (status == 200) { Serial.println(F("OK")); return true; }
  Serial.print(F("Fallo ")); Serial.println(status);
  return false;
}

void procesarYSubir(String data) {
  radio.sleep();
  delay(200);

  int p1 = data.indexOf('|');
  int p2 = data.indexOf('|', p1+1);
  int p3 = data.indexOf('|', p2+1);
  int p4 = data.indexOf('|', p3+1);
  if(p4 == -1) { Serial.println(F("Datos LoRa corruptos")); radio.startReceive(); return; }

  String loc = data.substring(0, p1);
  float temp = data.substring(p1+1, p2).toFloat();
  float pres = data.substring(p2+1, p3).toFloat();
  float hum  = data.substring(p3+1, p4).toFloat();
  float gas  = data.substring(p4+1).toFloat();

  if (conectarSTA()) {
    // Asegurar Token
    if (fbIdToken == "" || millis() > tokenExpiry) {
      if (!signIn()) {
        Serial.println(F("Abortando por falta de Auth"));
        WiFi.disconnect();
        radio.startReceive();
        return;
      }
    }
    
    // Envíos
    enviarAFirebase("temp_01", temp, loc);
    delay(300);
    enviarAFirebase("pres_01", pres, loc);
    delay(300);
    enviarAFirebase("hum_01",  hum,  loc);
    delay(300);
    enviarAFirebase("gas_01",  gas,  loc);
    
    Serial.println(F(">>> CICLO COMPLETADO"));
    delay(500);
  }
  
  Serial.println(F("--- Regresando a LoRa ---"));
  radio.startReceive();
}

void setup() {
  Serial.begin(115200);
  while(!Serial); // Esperar a monitor serial
  Serial.println(F("\n=== RECEPTOR R4 INICIADO ==="));

  int state = radio.begin(FRECUENCIA);
  if (state == RADIOLIB_ERR_NONE) {
    radio.setSyncWord(SYNC_WORD);
    Serial.println(F("LoRa: LISTO"));
  } else {
    Serial.print(F("Error LoRa: ")); Serial.println(state);
    while (true);
  }
}

void loop() {
  String mensaje;
  int state = radio.receive(mensaje);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("\n[LoRa] Mensaje recibido!"));
    procesarYSubir(mensaje);
  }
}