#include "web.h"

#if FASE2_WEB

#include <WiFi.h>
#include <WebServer.h>
#include "sensores.h"
#include "sd_log.h"

static WebServer server(80);

void handleLeituras() {
  String json = "{";
  json += "\"ph\":" + String(dados.ph, 2) + ",";
  json += "\"tds_ppm\":" + String(dados.tds_ppm, 1) + ",";
  json += "\"turbidez_ntu\":" + String(dados.turbidez_ntu, 1) + ",";
  json += "\"temp_c\":" + String(dados.temp_c, 1);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRegistrar() {
  gravarLinha("web");
  server.send(200, "application/json", "{\"ok\":true}");
}

void inicializarWeb() {
  WiFi.begin();
  server.on("/leituras", HTTP_GET, handleLeituras);
  server.on("/registrar", HTTP_POST, handleRegistrar);
  server.begin();
}

void servirWeb() {
  server.handleClient();
}

#endif
