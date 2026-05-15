/*
 * ARDUINO UNO R4 WiFi — BOT TELEGRAM ArgusLink
 * -----------------------------------------------
 * Rol: lee datos de Firebase RTDB y los envía por Telegram.
 *
 * Funciones:
 *   · Cada REPORT_INTERVAL: envía resumen de todos los sensores
 *     (última lectura de cada uno)
 *   · Cada ALARM_CHECK_INTERVAL: comprueba alarmas configuradas
 *     y envía alerta si se supera/baja del umbral (con cooldown)
 *
 * Configuración necesaria:
 *   1. Crea un bot en Telegram hablando con @BotFather → /newbot
 *      → te da el TOKEN
 *   2. Obtén tu CHAT_ID:
 *      a. Habla con tu bot (escríbele cualquier cosa)
 *      b. Abre en navegador:
 *         https://api.telegram.org/bot<TOKEN>/getUpdates
 *      c. Busca "chat":{"id": 123456789} → ese es tu CHAT_ID
 *
 * Librerías (Library Manager):
 *   - ArduinoHttpClient  (por Arduino)
 *   - ArduinoJson        (por Benoit Blanchon)
 */

#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// ============================================================
//  CONFIGURACIÓN — AJUSTA ESTOS VALORES
// ============================================================

// WiFi con acceso a internet
const char WIFI_SSID[]  = "Mr.Robot";
const char WIFI_PASS[]  = "Cw4cL.Ikyd";

// Firebase
const char FB_HOST[]    = "arguslink-default-rtdb.europe-west1.firebasedatabase.app";
const char FB_API_KEY[] = "AIzaSyCaFxyDqldwG48IqOrjmZjqxH6THaFqA98";
const char FB_EMAIL[]   = "arguslink10@gmail.com";
const char FB_PASSWORD[]= "avelino2000";

// Telegram
// TOKEN: lo que te da @BotFather (ej. "7123456789:AAFxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
const char TG_TOKEN[]   = "8657938432:AAFL7-CWTtmCs5DF7ndOdf-Sv_5wiOa2i6s";
// CHAT_ID: tu ID de usuario o grupo (ej. "123456789" o "-1001234567890" para grupo)
const char TG_CHAT_ID[] = "-1003734937541";

// Intervalo de reporte periódico (ms)
#define REPORT_INTERVAL      60000UL    // 1 minuto

// Intervalo de comprobación de alarmas (ms)
#define ALARM_CHECK_INTERVAL 30000UL    // cada 30 s

// Cooldown entre alertas del mismo sensor (ms) — evita spam
#define ALARM_COOLDOWN       600000UL   // 10 minutos

// ============================================================
//  ALARMAS — define aquí tus umbrales
//  { "sensorId", umbral, superarUmbral }
//    superarUmbral = true  → alerta si valor > umbral
//    superarUmbral = false → alerta si valor < umbral
// ============================================================
struct DefAlarma {
  const char* sensorId;
  float       umbral;
  bool        superarUmbral;
  const char* descripcion;    // texto que aparece en el mensaje
};

const DefAlarma ALARMAS[] = {
  { "temp_01", 26.0f, true,  "Temperatura ALTA"   },
  { "temp_01",  5.0f, false, "Temperatura BAJA"   },
  { "hum_01",  90.0f, true,  "Humedad ALTA"       },
  { "hum_01",  20.0f, false, "Humedad BAJA"       },
  { "gas_01",  700.0f, true, "Calidad aire MALA"  },
};
const int N_ALARMAS = sizeof(ALARMAS) / sizeof(ALARMAS[0]);

// Cooldown por alarma (se rellena en runtime)
unsigned long alarmaUltimaAlerta[N_ALARMAS] = {0};

// ============================================================

// ---- Token Firebase ----
String        fbIdToken      = "";
String        fbRefreshToken = "";
unsigned long tokenExpiry    = 0;

// ---- Timers ----
unsigned long ultimoReport     = 0;
unsigned long ultimoAlarmCheck = 0;
bool          primeraVez       = true;

// ---------------------------------------------------------------
// Utilidad: parsea campo String de JSON con o sin espacio tras ':'
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

// ---------------------------------------------------------------
// Firebase Auth
// ---------------------------------------------------------------
bool signIn() {
  Serial.print(F("[Auth] signIn... "));
  WiFiSSLClient ssl;
  HttpClient http(ssl, "identitytoolkit.googleapis.com", 443);
  http.setHttpResponseTimeout(10000);

  String path = "/v1/accounts:signInWithPassword?key=" + String(FB_API_KEY);
  String body = "{\"email\":\""   + String(FB_EMAIL)    + "\","
                "\"password\":\"" + String(FB_PASSWORD) + "\","
                "\"returnSecureToken\":true}";

  http.post(path, "application/json", body);
  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  if (status != 200) {
    Serial.print(F("ERROR status=")); Serial.println(status);
    return false;
  }
  fbIdToken      = authField(resp, "idToken");
  fbRefreshToken = authField(resp, "refreshToken");
  String exp     = authField(resp, "expiresIn");
  tokenExpiry    = millis() + (exp.toInt() - 60) * 1000UL;

  Serial.println(fbIdToken.isEmpty() ? F("FALLO — token vacío") : F("OK"));
  return !fbIdToken.isEmpty();
}

bool renovarToken() {
  Serial.print(F("[Auth] refresh... "));
  WiFiSSLClient ssl;
  HttpClient http(ssl, "securetoken.googleapis.com", 443);
  http.setHttpResponseTimeout(10000);

  String path = "/v1/token?key=" + String(FB_API_KEY);
  String body = "grant_type=refresh_token&refresh_token=" + fbRefreshToken;

  http.post(path, "application/x-www-form-urlencoded", body);
  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  if (status != 200) { Serial.println(F("ERROR")); return false; }
  fbIdToken      = authField(resp, "id_token");
  fbRefreshToken = authField(resp, "refresh_token");
  String exp     = authField(resp, "expires_in");
  tokenExpiry    = millis() + (exp.toInt() - 60) * 1000UL;

  Serial.println(fbIdToken.isEmpty() ? F("FALLO") : F("OK"));
  return !fbIdToken.isEmpty();
}

bool ensureToken() {
  if (fbIdToken.isEmpty())     return signIn();
  if (millis() >= tokenExpiry) return renovarToken();
  return true;
}

// ---------------------------------------------------------------
// GET Firebase RTDB
// ---------------------------------------------------------------
String fbGet(const String& path) {
  WiFiSSLClient ssl;
  HttpClient fb(ssl, FB_HOST, 443);
  fb.setHttpResponseTimeout(10000);
  String sep = (path.indexOf('?') >= 0) ? "&" : "?";
  fb.get(path + sep + "auth=" + fbIdToken);
  int    status = fb.responseStatusCode();
  String body   = fb.responseBody();
  fb.stop();
  if (status != 200) {
    Serial.print(F("[fbGet] ERROR status=")); Serial.print(status);
    Serial.print(F(" path=")); Serial.println(path);
    return "null";
  }
  return body;
}

// ---------------------------------------------------------------
// Telegram: enviar mensaje de texto
// Escapa < > & para HTML básico; usa parse_mode HTML para negritas
// ---------------------------------------------------------------
bool tgSend(const String& texto) {
  Serial.print(F("[Telegram] enviando... "));

  WiFiSSLClient ssl;
  HttpClient tg(ssl, "api.telegram.org", 443);
  tg.setHttpResponseTimeout(10000);

  String path = "/bot";
  path += TG_TOKEN;
  path += "/sendMessage";

  // Construir JSON del cuerpo
  // Escapar comillas del texto (por si hay unidades o nombres con ")
  String textoEsc = texto;
  textoEsc.replace("\"", "'");

  String body = "{\"chat_id\":\"";
  body += TG_CHAT_ID;
  body += "\",\"text\":\"";
  body += textoEsc;
  body += "\",\"parse_mode\":\"HTML\"}";

  tg.post(path, "application/json", body);
  int    status = tg.responseStatusCode();
  String resp   = tg.responseBody();
  tg.stop();

  if (status != 200) {
    Serial.print(F("ERROR status=")); Serial.println(status);
    Serial.print(F("  resp: ")); Serial.println(resp.substring(0, 100));
    return false;
  }
  Serial.println(F("OK"));
  return true;
}

// ---------------------------------------------------------------
// Timestamp Unix ms → "dd/mm/aaaa HH:MM" (UTC+2 España verano)
// ---------------------------------------------------------------
String formatTs(long long tsMs) {
  if (tsMs <= 0) return "--";
  unsigned long ts = (unsigned long)(tsMs / 1000UL) + 7200UL;   // UTC+2
  unsigned long t2 = ts % 86400UL;
  int hh = t2 / 3600, mm2 = (t2 % 3600) / 60;
  unsigned long days = ts / 86400UL;
  days += 719468;
  unsigned long era = days / 146097;
  unsigned long doe = days - era * 146097;
  unsigned long yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  unsigned long doy = doe - (365*yoe + yoe/4 - yoe/100);
  unsigned long mp  = (5*doy + 2) / 153;
  int day   = doy - (153*mp + 2) / 5 + 1;
  int month = mp + (mp < 10 ? 3 : -9);
  int year  = (int)(yoe + era * 400) + (month <= 2 ? 1 : 0);
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d", day, month, year, hh, mm2);
  return String(buf);
}

// ---------------------------------------------------------------
// Lee los sensores + última lectura de cada uno desde Firebase
// Devuelve false si no hay datos
// ---------------------------------------------------------------
bool leerSensores(String& mensajeOut) {
  if (!ensureToken()) return false;

  String sensJson = fbGet("/sensors.json");
  if (sensJson == "null" || sensJson.length() < 3) {
    Serial.println(F("[Report] sin sensores"));
    return false;
  }

  JsonDocument sensDoc;
  if (deserializeJson(sensDoc, sensJson)) return false;
  JsonObject sensObj = sensDoc.as<JsonObject>();

  mensajeOut = "<b>📊 ArgusLink — Informe de sensores</b>\n";

  for (JsonPair kv : sensObj) {
    String sid      = kv.key().c_str();
    JsonObject sv   = kv.value().as<JsonObject>();
    String name     = sv["name"]     | sid;
    String unit     = sv["unit"]     | "";
    String location = sv["location"] | "";

    mensajeOut += "\n<b>" + name + "</b>";
    if (location.length()) mensajeOut += " (" + location + ")";
    mensajeOut += "\n";

    // Última lectura
    String rJson = fbGet("/readings/" + sid + ".json?orderBy=%22%24key%22&limitToLast=1");
    if (rJson == "null" || rJson.length() < 3) {
      mensajeOut += "  Sin lecturas\n";
      continue;
    }

    JsonDocument rDoc;
    if (deserializeJson(rDoc, rJson)) { mensajeOut += "  Error JSON\n"; continue; }
    JsonObject rObj = rDoc.as<JsonObject>();

    for (JsonPair rk : rObj) {   // solo hay 1 entrada
      float     val = rk.value()["value"] | 0.0f;
      long long ts  = rk.value()["ts"]    | (long long)0;
      mensajeOut += "  Valor: <b>" + String(val, 2);
      if (unit.length()) mensajeOut += " " + unit;
      mensajeOut += "</b>\n";
      mensajeOut += "  Hora: " + formatTs(ts) + "\n";
    }
  }

  return true;
}

// ---------------------------------------------------------------
// Comprueba alarmas: para cada sensor lee la última lectura
// y envía alerta si se supera el umbral (respetando cooldown)
// ---------------------------------------------------------------
void comprobarAlarmas() {
  if (!ensureToken()) return;

  unsigned long ahora = millis();

  for (int a = 0; a < N_ALARMAS; a++) {
    // Cooldown activo → saltar (si alarmaUltimaAlerta==0 es la primera vez → pasar siempre)
    if (alarmaUltimaAlerta[a] != 0 && ahora - alarmaUltimaAlerta[a] < ALARM_COOLDOWN) continue;

    String sid   = String(ALARMAS[a].sensorId);
    String rJson = fbGet("/readings/" + sid + ".json?orderBy=%22%24key%22&limitToLast=1");

    if (rJson == "null" || rJson.length() < 3) continue;

    JsonDocument rDoc;
    if (deserializeJson(rDoc, rJson)) continue;
    JsonObject rObj = rDoc.as<JsonObject>();

    for (JsonPair rk : rObj) {
      float val = rk.value()["value"] | 0.0f;
      bool  disparar = ALARMAS[a].superarUmbral ? (val > ALARMAS[a].umbral)
                                                 : (val < ALARMAS[a].umbral);
      if (!disparar) continue;

      // Obtener nombre y unidad del sensor (una sola petición)
      String nombre = sid;
      String unidad = "";
      String sJson  = fbGet("/sensors/" + sid + ".json");
      if (sJson != "null" && sJson.length() > 2) {
        JsonDocument sDoc;
        if (!deserializeJson(sDoc, sJson)) {
          nombre = sDoc["name"] | sid;
          unidad = sDoc["unit"] | "";
        }
      }

      String msg = "🚨 <b>ALARMA — ";
      msg += ALARMAS[a].descripcion;
      msg += "</b>\n";
      msg += "Sensor: <b>" + nombre + "</b>\n";
      msg += "Valor actual: <b>" + String(val, 2);
      if (unidad.length()) msg += " " + unidad;
      msg += "</b>\n";
      msg += "Umbral: " + String(ALARMAS[a].umbral, 2);
      if (unidad.length()) msg += " " + unidad;
      msg += " (";
      msg += ALARMAS[a].superarUmbral ? "máximo" : "mínimo";
      msg += ")\n";
      msg += "Hora: " + formatTs(rk.value()["ts"] | (long long)0);

      Serial.print(F("[Alarma] ")); Serial.println(ALARMAS[a].descripcion);
      if (tgSend(msg)) {
        alarmaUltimaAlerta[a] = ahora;
      }
    }
  }
}

// ---------------------------------------------------------------
// Reconexión WiFi si se cae
// ---------------------------------------------------------------
void verificarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print(F("WiFi caído — reconectando"));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" OK"));
    delay(2000);
  } else {
    Serial.println(F(" FALLO — se reintentará en el siguiente ciclo"));
  }
}

// ---------------------------------------------------------------
// Setup
// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=== Servidor Telegram ArgusLink ==="));

  // WiFi
  Serial.print(F("Conectando a '")); Serial.print(WIFI_SSID); Serial.print(F("'"));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" OK"));
    Serial.print(F("  IP: ")); Serial.println(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println(F(" FALLO — se reintentará en loop()"));
    return;
  }

  // Firebase Auth
  for (int i = 1; i <= 3 && fbIdToken.isEmpty(); i++) {
    if (i > 1) delay(3000);
    signIn();
  }
  if (fbIdToken.isEmpty()) {
    Serial.println(F("AVISO: sin token Firebase — se reintentará en el primer reporte"));
  }

  // Mensaje de inicio por Telegram
  tgSend("<b>✅ ArgusLink Bot iniciado</b>\nReporte cada " +
         String(REPORT_INTERVAL / 1000) + " s · Alarmas activas: " +
         String(N_ALARMAS));

  // Primer reporte inmediato
  ultimoReport     = millis() - REPORT_INTERVAL;
  ultimoAlarmCheck = millis() - ALARM_CHECK_INTERVAL;
}

// ---------------------------------------------------------------
// Loop
// ---------------------------------------------------------------
void loop() {
  verificarWiFi();
  if (WiFi.status() != WL_CONNECTED) { delay(5000); return; }

  unsigned long ahora = millis();

  // ── Reporte periódico ───────────────────────────────────────
  if (ahora - ultimoReport >= REPORT_INTERVAL) {
    ultimoReport = ahora;
    Serial.println(F("\n--- Reporte periódico ---"));
    String msg;
    if (leerSensores(msg)) {
      tgSend(msg);
    } else {
      Serial.println(F("  Sin datos para reportar"));
    }
  }

  // ── Comprobación de alarmas ─────────────────────────────────
  if (ahora - ultimoAlarmCheck >= ALARM_CHECK_INTERVAL) {
    ultimoAlarmCheck = ahora;
    Serial.println(F("\n--- Comprobando alarmas ---"));
    comprobarAlarmas();
  }
}
 