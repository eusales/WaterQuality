#include "maquina.h"
#include "display.h"
#include "nvs_cal.h"
#include "sd_log.h"
#include "buzzer.h"

static Estado estadoAtual = MONITORAMENTO;

int   menuSel     = 0;
int   calibSel    = 0;
int   calibPasso  = 0;
int   rotuloSel   = 0;
int   histSel     = 0;
int   confSel     = 0;
bool  confEditando = false;

float lim_ph_min  = PH_MIN_DEFAULT;
float lim_ph_max  = PH_MAX_DEFAULT;
float lim_tds     = TDS_LIMIT_DEFAULT;
float lim_turb    = TURB_LIMIT_DEFAULT;
int   lim_interv  = 2;

#define NUM_MENU    4
#define NUM_CALIB   3
#define NUM_ROTULOS 5
#define NUM_CONF    6

static float calibV7 = 0, calibV4 = 0, calibV10 = 0;
static float calibV0 = 0, calibV50 = 0, calibV500 = 0;

Estado getEstado() {
  return estadoAtual;
}

void acaoNavegar() {
  switch (estadoAtual) {
    case MENU_PRINCIPAL:
      menuSel = (menuSel + 1) % NUM_MENU;
      break;
    case CALIBRACAO_MENU:
      calibSel = (calibSel + 1) % NUM_CALIB;
      break;
    case REGISTRO:
      rotuloSel = (rotuloSel + 1) % NUM_ROTULOS;
      break;
    case HISTORICO:
      histSel = (histSel + 1) % 5;
      break;
    case CONFIGURACOES:
      if (confEditando) {
        switch (confSel) {
          case 0: lim_ph_min = min(lim_ph_min + 0.1f, lim_ph_max - 0.1f); break;
          case 1: lim_ph_max = min(lim_ph_max + 0.1f, 14.0f); break;
          case 2: lim_tds    = min(lim_tds + 50.0f, 5000.0f); break;
          case 3: lim_turb   = min(lim_turb + 10.0f, 1000.0f); break;
          case 4: lim_interv = min(lim_interv + 1, 60); break;
        }
      } else {
        confSel = (confSel + 1) % NUM_CONF;
      }
      break;
    default:
      break;
  }
  desenharTela(estadoAtual);
}

static void processarCalibracao() {
  int total = (calibSel == 1) ? 1 : 3;
  float vAtual = dados.ph;

  if (calibSel == 0) {
    if (calibPasso == 0) calibV7  = vAtual;
    else if (calibPasso == 1) calibV4  = vAtual;
    else if (calibPasso == 2) calibV10 = vAtual;
  } else if (calibSel == 2) {
    if (calibPasso == 0) calibV0   = vAtual;
    else if (calibPasso == 1) calibV50  = vAtual;
    else if (calibPasso == 2) calibV500 = vAtual;
  }

  if (calibPasso < total - 1) {
    calibPasso++;
  } else {
    if (calibSel == 0) {
      calib.ph_offset    = calibV7;
      calib.ph_slope_acid = (7.0f - 4.0f) / (calibV7 - calibV4);
      calib.ph_slope_bas  = (7.0f - 10.0f) / (calibV7 - calibV10);
    } else if (calibSel == 1) {
      float ec25 = (vAtual * 1000.0f / 2.3f) / (1.0f + 0.02f * (dados.temp_c - 25.0f));
      calib.k_tds = 1413.0f / ec25;
    } else if (calibSel == 2) {
      float x0 = calibV0, x1 = calibV50, x2 = calibV500;
      float y0 = 0, y1 = 50, y2 = 500;
      calib.turb_a = ((y1 - y0) / (x1 - x0) - (y2 - y0) / (x2 - x0)) / (x1 - x2);
      calib.turb_b = (y1 - y0) / (x1 - x0) - calib.turb_a * (x0 + x1);
      calib.turb_c = y0 - calib.turb_a * x0 * x0 - calib.turb_b * x0;
    }
    salvarCalibracao();
    bipConfirmacao();
    estadoAtual = CALIBRACAO_MENU;
  }
}

void acaoConfirmar() {
  switch (estadoAtual) {
    case MONITORAMENTO:
      estadoAtual = MENU_PRINCIPAL;
      menuSel = 0;
      break;

    case MENU_PRINCIPAL:
      switch (menuSel) {
        case 0: estadoAtual = CALIBRACAO_MENU; calibSel = 0; break;
        case 1: estadoAtual = HISTORICO;       histSel  = 0; break;
        case 2: estadoAtual = CONFIGURACOES;   confSel  = 0; confEditando = false; break;
        case 3: estadoAtual = REGISTRO;        rotuloSel = 0; break;
      }
      break;

    case CALIBRACAO_MENU:
      estadoAtual = CALIBRACAO_FLUXO;
      calibPasso = 0;
      break;

    case CALIBRACAO_FLUXO:
      processarCalibracao();
      break;

    case REGISTRO: {
      const char* rotulos[] = {"Destilada", "Torneira", "Rio", "Amostra1", "Amostra2"};
      gravarLinha(rotulos[rotuloSel]);
      bipConfirmacao();
      estadoAtual = MENU_PRINCIPAL;
      break;
    }

    case HISTORICO:
      break;

    case CONFIGURACOES:
      confEditando = !confEditando;
      if (!confEditando) {
        salvarCalibracao();
      }
      break;
  }
  desenharTela(estadoAtual);
}

void acaoVoltar() {
  switch (estadoAtual) {
    case MONITORAMENTO:    break;
    case MENU_PRINCIPAL:   estadoAtual = MONITORAMENTO;   break;
    case CALIBRACAO_MENU:  estadoAtual = MENU_PRINCIPAL;  break;
    case CALIBRACAO_FLUXO: estadoAtual = CALIBRACAO_MENU; break;
    case REGISTRO:         estadoAtual = MENU_PRINCIPAL;  break;
    case HISTORICO:        estadoAtual = MENU_PRINCIPAL;  break;
    case CONFIGURACOES:    confEditando = false; estadoAtual = MENU_PRINCIPAL; break;
  }
  desenharTela(estadoAtual);
}
