#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "display.h"
#include "botoes.h"
#include "maquina.h"
#include "nvs_cal.h"
#include "sd_log.h"
#include "buzzer.h"
#if FASE2_WEB
#include "web.h"
#endif

static unsigned long ultimaLeitura = 0;

void setup() {
  Serial.begin(115200);
  inicializarDisplay();
  desenharSplash();
  inicializarBotoes();
  inicializarSensores();
  carregarCalibracao();
  inicializarSD();
  inicializarBuzzer();
  desenharTela(MONITORAMENTO);
}

void loop() {
  verificarBotoes();

  unsigned long agora = millis();
  if (agora - ultimaLeitura >= DEFAULT_INTERVAL_MS) {
    ultimaLeitura = agora;
    lerTodos();
    if (getEstado() == MONITORAMENTO) {
      desenharTela(MONITORAMENTO);
    }
  }

  #if FASE2_WEB
  servirWeb();
  #endif
}
