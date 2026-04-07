#include <Arduino.h>
#include <WiFiS3.h>
#include "config.h"
#include "wifi_portal.h"

WiFiServer server(80);

static bool portalMessagePending = false;
static String portalMessage = "";

static void sendHtmlHeader(WiFiClient &client) {
 client.println("HTTP/1.1 200 OK");
 client.println("Content-Type: text/html; charset=UTF-8");
 client.println("Connection: close");
 client.println();
}

static void sendPortalPage(WiFiClient &client) {
 sendHtmlHeader(client);

 client.println("<!DOCTYPE html>");
 client.println("<html lang='es'>");
 client.println("<head>");
 client.println("<meta charset='UTF-8'>");
 client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
 client.println("<title>ARGUSLINK SOS</title>");
 client.println("</head>");
 client.println("<body>");
 client.println("<h1>ARGUSLINK SOS</h1>");
 client.println("<p>Escribe tu mensaje de emergencia:</p>");
 client.println("<form action='/e' method='get'>");
 client.println("<input type='text' name='m' maxlength='60'>");
 client.println("<input type='submit' value='Enviar SOS'>");
 client.println("</form>");
 client.println("</body>");
 client.println("</html>");
}

static void sendConfirmationPage(WiFiClient &client, const String &msg) {
 sendHtmlHeader(client);

 client.println("<!DOCTYPE html>");
 client.println("<html lang='es'>");
 client.println("<head><meta charset='UTF-8'><title>SOS enviado</title></head>");
 client.println("<body>");
 client.println("<h1>SOS enviado</h1>");
 client.println("<p>Mensaje registrado correctamente.</p>");
 client.println("<p><b>Contenido:</b> " + msg + "</p>");
 client.println("<a href='/'>Volver</a>");
 client.println("</body>");
 client.println("</html>");
}

static String decodeSimpleUrl(String text) {
 text.replace("+", " ");
 text.replace("%20", " ");
 text.replace("%3A", ":");
 text.replace("%2F", "/");
 return text;
}

void initWiFiPortal() {
 if (WiFi.status() == WL_NO_MODULE) {
   Serial.println("Error: módulo WiFi no detectado");
   while (true);
 }

 int status = WiFi.beginAP(WIFI_SSID, WIFI_PASS);
 if (status != WL_AP_LISTENING) {
   Serial.println("Error: no se pudo iniciar el punto de acceso");
   while (true);
 }

 delay(2000);
 server.begin();

 Serial.println("Portal WiFi iniciado");
 Serial.print("SSID: ");
 Serial.println(WiFi.SSID());
 Serial.print("IP local: ");
 Serial.println(WiFi.localIP());
}

void handlePortal() {
 WiFiClient client = server.available();
 if (!client) {
   return;
 }

 String requestLine = client.readStringUntil('\n');
 requestLine.trim();

 Serial.print("HTTP request: ");
 Serial.println(requestLine);

 if (requestLine.startsWith("GET /e?m=")) {
   int start = requestLine.indexOf("m=") + 2;
   int end = requestLine.indexOf(' ', start);

   String rawMessage = requestLine.substring(start, end);
   rawMessage = decodeSimpleUrl(rawMessage);

   portalMessage = "WEB: " + rawMessage;
   portalMessagePending = true;

   sendConfirmationPage(client, portalMessage);
 } else {
   sendPortalPage(client);
 }

 delay(1);
 client.stop();
}

bool hasPortalMessage() {
 return portalMessagePending;
}

String consumePortalMessage() {
 portalMessagePending = false;
 return portalMessage;
}