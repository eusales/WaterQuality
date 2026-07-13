#include "display.h"
#include "maquina.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define W 320
#define H 240

#define COR_FUNDO        0x0000
#define COR_HEADER       0x0A3F
#define COR_BRANCO       0xFFFF
#define COR_VERDE        0x07E0
#define COR_VERMELHO     0xF800
#define COR_AMARELO      0xFFE0
#define COR_CIANO        0x07FF
#define COR_CINZA        0x39E7
#define COR_CINZA_ESC    0x2104
#define COR_BTN_VERDE    0x03E0
#define COR_BTN_CINZA    0x4208
#define COR_SELECIONADO  0x0439
#define COR_ALERTA_FUNDO 0x2000

static Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── Helpers de desenho ───────────────────────────────────────
static void desenharHeader(const char* titulo, const char* info = nullptr) {
  tft.fillRect(0, 0, W, 22, COR_HEADER);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(1);
  tft.setCursor(6, 7);
  tft.print(titulo);
  if (info) {
    tft.setCursor(W - strlen(info) * 6 - 6, 7);
    tft.print(info);
  }
}

static void desenharBotao(int x, int y, int w, int h, const char* texto, uint16_t cor) {
  tft.fillRoundRect(x, y, w, h, 4, cor);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(1);
  int tw = strlen(texto) * 6;
  tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
  tft.print(texto);
}

static void desenharRodape(const char* btnEsq, uint16_t corEsq, const char* btnDir, uint16_t corDir) {
  int y = H - 26;
  tft.fillRect(0, y - 4, W, 4, COR_CINZA_ESC);
  desenharBotao(4, y, 150, 22, btnEsq, corEsq);
  desenharBotao(162, y, 154, 22, btnDir, corDir);
}

static void sepH(int y, uint16_t cor = COR_CINZA_ESC) {
  tft.drawFastHLine(0, y, W, cor);
}

// ── Inicialização ────────────────────────────────────────────
void inicializarDisplay() {
  tft.init(240, 320);
  tft.setRotation(3);
  tft.fillScreen(COR_FUNDO);
}

void desenharSplash() {
  tft.fillScreen(COR_FUNDO);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(2);
  tft.setCursor(40, 80);
  tft.print("Monitor de");
  tft.setCursor(60, 110);
  tft.print("Qualidade");
  tft.setCursor(90, 140);
  tft.print("da Agua");
  tft.setTextSize(1);
  tft.setTextColor(COR_CINZA);
  tft.setCursor(100, 180);
  tft.print("Inicializando...");
}

// Forward declaration
static void desenharTela3Fluxo();

// ── Despacho ─────────────────────────────────────────────────
void desenharTela(Estado e) {
  switch (e) {
    case MONITORAMENTO:    desenharTela1_Monitoramento(); break;
    case MENU_PRINCIPAL:   desenharTela2_Menu();          break;
    case CALIBRACAO_MENU:  desenharTela3_Calibracao();    break;
    case CALIBRACAO_FLUXO: desenharTela3Fluxo();          break;
    case REGISTRO:         desenharTela4_Registro();      break;
    case HISTORICO:        desenharTela5_Historico();     break;
    case CONFIGURACOES:    desenharTela6_Configuracoes(); break;
  }
}

// ── TELA 1: Monitoramento ───────────────────────────────────
void desenharTela1_Monitoramento() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Monitor Qualidade Agua", "*");

  int qw = W / 2;
  int qh = 70;
  int qy = 24;

  bool phOk  = dados.ph_ok;
  bool tdsOk = dados.tds_ok;

  tft.fillRect(0, qy, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, qy + 5); tft.print("pH");
  tft.setTextColor(phOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(6, qy + 18);
  char buf[16]; dtostrf(dados.ph, 4, 2, buf); tft.print(buf);

  tft.fillRect(qw + 1, qy, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(qw + 6, qy + 5); tft.print("TDS");
  tft.setTextColor(tdsOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(qw + 6, qy + 18);
  dtostrf(dados.tds_ppm, 6, 0, buf); tft.print(buf);
  tft.setTextSize(1); tft.print(" ppm");

  sepH(qy + qh);
  int qy2 = qy + qh + 2;

  bool turbOk = dados.turb_ok;
  tft.fillRect(0, qy2, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, qy2 + 5); tft.print("TURBIDEZ");
  tft.setTextColor(turbOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(6, qy2 + 18);
  dtostrf(dados.turbidez_ntu, 4, 1, buf); tft.print(buf);
  tft.setTextSize(1); tft.print(" NTU");

  tft.fillRect(qw + 1, qy2, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(qw + 6, qy2 + 5); tft.print("TEMP");
  tft.setTextColor(COR_BRANCO); tft.setTextSize(2);
  tft.setCursor(qw + 6, qy2 + 18);
  dtostrf(dados.temp_c, 4, 1, buf); tft.print(buf);
  tft.setTextSize(1); tft.print(" C");

  sepH(qy2 + qh);
  int alertY = qy2 + qh + 4;

  if (!tdsOk) {
    tft.fillRect(0, alertY, W, 16, COR_ALERTA_FUNDO);
    tft.setTextColor(COR_AMARELO); tft.setTextSize(1);
    tft.setCursor(6, alertY + 4);
    tft.print("! TDS acima do limite ("); tft.print((int)lim_tds); tft.print(" ppm)");
  } else if (!phOk) {
    tft.fillRect(0, alertY, W, 16, COR_ALERTA_FUNDO);
    tft.setTextColor(COR_AMARELO); tft.setTextSize(1);
    tft.setCursor(6, alertY + 4);
    tft.print("! pH fora do intervalo (");
    dtostrf(lim_ph_min, 3, 1, buf); tft.print(buf);
    tft.print(" - ");
    dtostrf(lim_ph_max, 3, 1, buf); tft.print(buf);
    tft.print(")");
  }

  desenharRodape("  Registrar", COR_BTN_CINZA, "  Menu >", COR_BTN_VERDE);
}

// ── TELA 2: Menu principal ──────────────────────────────────
void desenharTela2_Menu() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Menu");

  const char* itens[] = {"Calibracao", "Historico", "Configuracoes", "Registrar amostra"};

  for (int i = 0; i < 4; i++) {
    int y = 28 + i * 36;
    bool sel = (i == menuSel);
    tft.fillRect(2, y, W - 4, 32, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 8);
    tft.print(sel ? "> " : "  "); tft.print(itens[i]);
    tft.setCursor(W - 16, y + 8); tft.print(">");
    sepH(y + 32);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Entrar >", COR_BTN_VERDE);
}

// ── TELA 3A: Submenu Calibração ─────────────────────────────
void desenharTela3_Calibracao() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Calibracao - Selecione sensor");

  const char* itens[] = {"pH", "TDS", "Turbidez"};

  for (int i = 0; i < 3; i++) {
    int y = 30 + i * 50;
    bool sel = (i == calibSel);
    tft.fillRect(2, y, W - 4, 44, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 14);
    tft.print(sel ? "> " : "  "); tft.print(itens[i]);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Iniciar >", COR_BTN_VERDE);
}

// ── TELA 3B: Fluxo de calibração ────────────────────────────
static void desenharTela3Fluxo() {
  tft.fillScreen(COR_FUNDO);

  const char* sensores[] = {"pH", "TDS", "Turbidez"};
  const char* passosPH[]   = {"pH 7,00", "pH 4,00", "pH 10,01"};
  const char* passosTurb[] = {"Agua destilada (~0 NTU)", "Argila diluida (~50 NTU)", "Argila conc. (~500 NTU)"};

  int total = (calibSel == 1) ? 1 : 3;

  char hdr[32];
  snprintf(hdr, sizeof(hdr), "Calibracao - %s  %d/%d",
           sensores[calibSel], calibPasso + 1, total);
  desenharHeader(hdr);

  int progY = 26;
  tft.fillRect(4, progY, W - 8, 12, COR_CINZA_ESC);
  int progW = (int)((W - 8) * (calibPasso + 1) / (float)total);
  tft.fillRect(4, progY, progW, 12, COR_BTN_VERDE);
  int pct = (calibPasso + 1) * 100 / total;
  tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
  char pctBuf[8]; snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  tft.setCursor(W / 2 - 12, progY + 2); tft.print(pctBuf);

  int iy = 44;
  tft.setTextColor(COR_CIANO); tft.setTextSize(1);
  tft.setCursor(6, iy);

  if (calibSel == 0) {
    tft.print("Passo "); tft.print(calibPasso + 1); tft.print(" de 3 - Solucao tampao ");
    tft.print(passosPH[calibPasso]);
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe o eletrodo e aguarde...");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    tft.print("Aguardando...");
  } else if (calibSel == 1) {
    tft.print("Passo 1 de 1 - Solucao padrao 1413 uS/cm");
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe a sonda e o DS18B20.");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    tft.print("Aguardando...");
  } else {
    tft.print("Passo "); tft.print(calibPasso + 1); tft.print(" de 3 - ");
    tft.print(passosTurb[calibPasso]);
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe o sensor e aguarde...");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    tft.print("Aguardando...");
  }

  tft.setTextSize(1);
  if (calibPasso < total - 1) {
    tft.setTextColor(COR_CINZA); tft.setCursor(6, iy + 68);
    if (calibSel == 0) { tft.print("Proximo - "); tft.print(passosPH[calibPasso + 1]); }
    else if (calibSel == 2) { tft.print("Proximo - "); tft.print(passosTurb[calibPasso + 1]); }
    tft.setCursor(6, iy + 80); tft.setTextColor(COR_AMARELO);
    tft.print("Aguardando confirmacao...");
  } else {
    tft.setTextColor(COR_AMARELO); tft.setCursor(6, iy + 68);
    tft.print("Ultimo passo - Confirmar para salvar NVS");
  }

  const char* lblDir = (calibPasso < total - 1) ? "Proximo >" : "Salvar >";
  desenharRodape("X Cancelar", COR_BTN_CINZA, lblDir, COR_BTN_VERDE);
}

// ── TELA 4: Registro de amostra ─────────────────────────────
void desenharTela4_Registro() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Registrar Amostra");
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, 28); tft.print("Selecione o rotulo:");

  const char* rotulos[] = {"Agua destilada", "Agua torneira", "Rio / corrego", "Amostra 1", "Amostra 2"};

  for (int i = 0; i < 5; i++) {
    int y = 40 + i * 30;
    bool sel = (i == rotuloSel);
    tft.fillRect(2, y, W - 4, 26, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 9);
    tft.print(sel ? "> " : "  "); tft.print(rotulos[i]);
  }

  desenharRodape("X Cancelar", COR_BTN_CINZA, "Salvar >", COR_BTN_VERDE);
}

// ── TELA 5: Histórico ───────────────────────────────────────
void desenharTela5_Historico() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Historico - agua_log.csv");

  const char* linhas[] = {
    "14:32 15/06 | Torneira  | pH 7.02",
    "10:15 15/06 | Rio       | pH 6.12",
    "08:00 15/06 | Destilada | pH 7.00",
    "17:44 14/06 | Torneira  | pH 7.21",
    "09:10 14/06 | Amostra 1 | pH 5.80"
  };

  for (int i = 0; i < 5; i++) {
    int y = 26 + i * 36;
    bool sel = (i == histSel);
    tft.fillRect(2, y, W - 4, 32, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(sel ? COR_BRANCO : COR_CINZA); tft.setTextSize(1);
    tft.setCursor(6, y + 10);
    tft.print(linhas[i]);
    sepH(y + 32);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Ver >", COR_BTN_VERDE);
}

// ── TELA 6: Configurações ───────────────────────────────────
void desenharTela6_Configuracoes() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Configuracoes");

  const char* nomes[] = {
    "Limite pH min", "Limite pH max", "Limite TDS",
    "Limite turbidez", "Intervalo leitura", "Arquivo log"
  };

  for (int i = 0; i < 6; i++) {
    int y = 26 + i * 32;
    bool sel  = (i == confSel);
    bool edit = sel && confEditando;
    tft.fillRect(2, y, W - 4, 30, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(8, y + 10); tft.print(nomes[i]);
    char vbuf[24];
    tft.setTextColor(edit ? COR_AMARELO : (sel ? COR_VERDE : COR_CIANO));
    tft.setCursor(200, y + 10);
    if (edit) tft.print("< ");
    switch (i) {
      case 0: dtostrf(lim_ph_min, 4, 1, vbuf); tft.print(vbuf); break;
      case 1: dtostrf(lim_ph_max, 4, 1, vbuf); tft.print(vbuf); break;
      case 2: snprintf(vbuf, sizeof(vbuf), "%d ppm", (int)lim_tds); tft.print(vbuf); break;
      case 3: snprintf(vbuf, sizeof(vbuf), "%d NTU", (int)lim_turb); tft.print(vbuf); break;
      case 4: snprintf(vbuf, sizeof(vbuf), "%d s", lim_interv); tft.print(vbuf); break;
      case 5: tft.print("agua_log"); break;
    }
    if (edit) tft.print(" >");
    sepH(y + 30);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Salvar >", COR_BTN_VERDE);
}
