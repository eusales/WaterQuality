#include "botoes.h"
#include "maquina.h"

#define DEBOUNCE_MS    20
#define BTN_INTERVALO 200

static unsigned long ultimoNav  = 0;
static unsigned long ultimoConf = 0;
static unsigned long ultimoVolt = 0;

static bool lerBotao(int pino, unsigned long& ultimoMs) {
  if (digitalRead(pino) == LOW) {
    unsigned long agora = millis();
    if (agora - ultimoMs > BTN_INTERVALO) {
      delay(DEBOUNCE_MS);
      if (digitalRead(pino) == LOW) {
        ultimoMs = agora;
        return true;
      }
    }
  }
  return false;
}

void inicializarBotoes() {
  pinMode(BTN_NAV,  INPUT_PULLUP);
  pinMode(BTN_CONF, INPUT_PULLUP);
  pinMode(BTN_VOLT, INPUT_PULLUP);
}

void verificarBotoes() {
  if (lerBotao(BTN_NAV,  ultimoNav))  acaoNavegar();
  if (lerBotao(BTN_CONF, ultimoConf)) acaoConfirmar();
  if (lerBotao(BTN_VOLT, ultimoVolt)) acaoVoltar();
}
