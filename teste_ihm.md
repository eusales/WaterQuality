# Teste — IHM (Interface Homem-Máquina)

> **Projeto:** Monitor Portátil de Qualidade da Água  
> **Disciplina:** Instrumentação Industrial II — UFMT  
> **Plataforma:** ESP32 DevKit V3 (WROOM-32)

---

## 1. Objetivo

Validar a navegação entre as 6 telas da IHM com os 3 botões físicos e verificar a renderização correta do display TFT ST7789 em orientação paisagem (landscape). Os sensores **não são utilizados** neste teste — todos os valores exibidos são simulados por constantes no firmware.

---

## 2. Bibliotecas

| Biblioteca | Versão mínima | Instalação |
|---|---|---|
| Adafruit ST7789 | ≥ 1.9.0 | Arduino IDE → Gerenciador de Bibliotecas |
| Adafruit GFX Library | ≥ 1.11.0 | Arduino IDE → Gerenciador de Bibliotecas |
| SPI | built-in ESP32 | Já incluída no core ESP32 |

> Nenhuma biblioteca de sensor (ADS1115, OneWire, DallasTemperature) é necessária neste teste.

---

## 3. Pinout

### Botões físicos

| Botão | GPIO | Pino físico | Ligação |
|---|---|---|---|
| Navegar | GPIO 32 | 7 | GPIO32 → GND |
| Confirmar | GPIO 33 | 8 | GPIO33 → GND |
| Voltar | GPIO 25 | 10 | GPIO25 → GND |

**Configuração por software:** `INPUT_PULLUP` — sem resistor externo necessário.  
**Debounce:** 20 ms por software. Anti-repetição de 200 ms entre pressões aceitas.

> ⚠ GPIOs 32, 33 e 25 são ADC/TOUCH livres — sem conflito com SPI (TFT), I²C (ADS1115) ou 1-Wire (DS18B20).

### TFT ST7789 2.0"

| GPIO | Pino físico | Pino TFT | Observação |
|---|---|---|---|
| GPIO 18 | 30 | SCL (SCK) | Barramento SPI |
| GPIO 23 | 37 | SDA (MOSI) | Barramento SPI |
| GPIO 5 | 29 | CS | Chip Select TFT |
| GPIO 2 | 24 | DC | Data/Command |
| GPIO 4 | 26 | RST | Reset |
| 3.3V | 1 | VCC | Alimentação |
| 3.3V | 1 | BLK | Backlight fixo ligado |
| GND | 14 | GND | |

> GPIO 19 (MISO) não é utilizado neste teste — o SD card não é conectado.

---

## 4. Telas e navegação

### Mapa de estados

| Estado | Navegar | Confirmar | Voltar |
|---|---|---|---|
| Tela 1 — Monitoramento | sem efeito | → Menu principal | sem efeito (nível raiz) |
| Tela 2 — Menu principal | percorre itens (circular) | → item selecionado | → Monitoramento |
| Tela 3 — Submenu Calibração | percorre sensores (circular) | → fluxo do sensor | → Menu principal |
| Tela 3 — Fluxo calibração | sem efeito | avança passo / salva no último | → Submenu Calibração |
| Tela 4 — Registro | percorre rótulos (circular) | exibe confirmação → Menu | → Menu principal |
| Tela 5 — Histórico | percorre registros (circular) | exibe detalhe | → Menu principal |
| Tela 6 — Configurações | seleciona parâmetro / incrementa valor em edição | alterna modo edição | → Menu principal |

### Descrição de cada tela

| Tela | Conteúdo exibido |
|---|---|
| 1 — Monitoramento | pH, TDS, Turbidez e Temperatura em quadrantes 2×2. Valores em verde (OK) ou vermelho (fora do limite). Alerta na faixa inferior quando algum limite é excedido. |
| 2 — Menu principal | Lista de 4 itens com marcador ◉/○. Item selecionado destacado em azul. |
| 3 — Calibração | Submenu de seleção de sensor (pH / TDS / Turbidez) e fluxo passo a passo com barra de progresso, tensão simulada e indicador de confirmação. |
| 4 — Registro de amostra | Lista de 5 rótulos com marcador ◉/○. Ao confirmar, exibe snapshot simulado por 2,5 s. |
| 5 — Histórico | 5 registros simulados com data/hora, rótulo e pH colorido por alerta. Confirmar exibe detalhe completo do registro selecionado. |
| 6 — Configurações | 6 parâmetros configuráveis. Item em edição destacado com `< valor >`. Navegar incrementa o valor enquanto em modo edição. |

---

## 5. Sequência do teste

| Etapa | Ação | Resultado esperado |
|---|---|---|
| 1 | Energizar o ESP32 | Tela de boot exibida por 2 s, depois Tela 1 — Monitoramento |
| 2 | Observar Tela 1 | pH verde (7,02), TDS vermelho (1142 ppm), alerta visível na faixa inferior |
| 3 | Pressionar Confirmar | Vai para Tela 2 — Menu principal, "Calibração" selecionado |
| 4 | Pressionar Navegar 3× | Percorre: Histórico → Configurações → Registrar amostra (circular) |
| 5 | Pressionar Voltar | Volta para Tela 1 — Monitoramento |
| 6 | Confirmar → Confirmar (entra em Calibração) | Tela 3 — Submenu Calibração, "pH" selecionado |
| 7 | Confirmar (inicia fluxo pH) | Tela 3 — Passo 1/3, barra de progresso em 33% |
| 8 | Confirmar (avança) | Passo 2/3, barra em 66% |
| 9 | Confirmar (avança) | Passo 3/3, barra em 100% |
| 10 | Confirmar (salva) | Exibe "SALVO OK!" por 2 s, volta ao submenu |
| 11 | Voltar → Navegar 3× → Confirmar (entra em Registrar) | Tela 4 — Registro de amostra |
| 12 | Navegar 2× → Confirmar | Seleciona "Rio / córrego", exibe confirmação por 2,5 s |
| 13 | Confirmar → Navegar → Confirmar (entra em Histórico) | Tela 5 — Histórico |
| 14 | Navegar e observar seleção | pH fora do limite aparece em vermelho |
| 15 | Confirmar | Exibe detalhe do registro selecionado |
| 16 | Pressionar Voltar | Retorna ao Histórico |
| 17 | Voltar → Navegar 2× → Confirmar (entra em Configurações) | Tela 6 — Configurações |
| 18 | Navegar até "Limite TDS" → Confirmar | Entra em modo edição (`< 1000 ppm >`) |
| 19 | Navegar 3× | Valor incrementa para 1150 ppm |
| 20 | Confirmar | Sai do modo edição, valor atualizado |
| 21 | Voltar | Retorna ao Menu principal |

---

## 6. Código completo

```cpp
// ============================================================
// TESTE IHM — Monitor de Qualidade da Água
// ESP32 DevKit V3 + TFT ST7789 2.0" (Adafruit, landscape 320x240)
// Dados SIMULADOS — sem sensores reais
// Objetivo: validar navegação, botões e renderização das 6 telas
//
// Botões (INPUT_PULLUP — LOW = pressionado):
//   Navegar   = GPIO 32
//   Confirmar = GPIO 33
//   Voltar    = GPIO 25
//
// TFT SPI (VSPI):
//   SCK=18, MOSI=23, CS=5, DC=2, RST=4
// ============================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ── Pinos TFT ────────────────────────────────────────────────
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// ── Botões ───────────────────────────────────────────────────
#define BTN_NAVEGAR   32
#define BTN_CONFIRMAR 33
#define BTN_VOLTAR    25
#define DEBOUNCE_MS   20

// ── Dimensões ────────────────────────────────────────────────
#define W 320
#define H 240

// ── Cores ────────────────────────────────────────────────────
#define COR_FUNDO       0x0000   // Preto
#define COR_HEADER      0x0A3F   // Azul escuro
#define COR_BRANCO      0xFFFF
#define COR_VERDE       0x07E0
#define COR_VERMELHO    0xF800
#define COR_AMARELO     0xFFE0
#define COR_CIANO       0x07FF
#define COR_CINZA       0x39E7
#define COR_CINZA_ESC   0x2104
#define COR_BTN_VERDE   0x03E0
#define COR_BTN_CINZA   0x4208
#define COR_SELECIONADO 0x0439
#define COR_ALERTA_FUNDO 0x2000

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── Estados da máquina ───────────────────────────────────────
enum Estado {
  MONITORAMENTO,
  MENU_PRINCIPAL,
  CALIBRACAO_MENU,
  CALIBRACAO_FLUXO,
  REGISTRO,
  HISTORICO,
  CONFIGURACOES
};

Estado estadoAtual = MONITORAMENTO;

// ── Dados simulados ──────────────────────────────────────────
float sim_ph   = 7.02f;
float sim_tds  = 1142.0f;
float sim_turb = 4.1f;
float sim_temp = 24.3f;

// Limites (configuráveis)
float lim_ph_min  = 6.0f;
float lim_ph_max  = 9.0f;
float lim_tds     = 1000.0f;
float lim_turb    = 100.0f;
int   lim_interv  = 2;

// ── Menu principal ───────────────────────────────────────────
const char* itensMenu[] = {"Calibracao", "Historico", "Configuracoes", "Registrar amostra"};
const int   NUM_MENU = 4;
int menuSel = 0;

// ── Calibração ───────────────────────────────────────────────
const char* itensCalib[] = {"pH", "TDS", "Turbidez"};
const int   NUM_CALIB = 3;
int calibSel  = 0;
int calibPasso = 0;

const char* passosCalibPH[]   = {"pH 7,00", "pH 4,00", "pH 10,01"};
const char* passosCalibTurb[] = {"Agua destilada (~0 NTU)", "Argila diluida (~50 NTU)", "Argila conc. (~500 NTU)"};
float valoresCalib[3] = {0, 0, 0};

int getTotalPassos() {
  if (calibSel == 0) return 3;
  if (calibSel == 1) return 1;
  return 3;
}

// ── Registro ─────────────────────────────────────────────────
const char* rotulos[] = {"Agua destilada", "Agua torneira", "Rio / corrego", "Amostra 1", "Amostra 2"};
const int   NUM_ROTULOS = 5;
int rotuloSel = 0;

// ── Histórico simulado ───────────────────────────────────────
struct Registro {
  const char* hora;
  const char* rotulo;
  float ph;
  float tds;
};
Registro historico[] = {
  {"14:32 15/06", "Torneira",  7.02f, 312.0f},
  {"10:15 15/06", "Rio",       6.12f,  88.0f},
  {"08:00 15/06", "Destilada", 7.00f,   1.0f},
  {"17:44 14/06", "Torneira",  7.21f, 298.0f},
  {"09:10 14/06", "Amostra 1", 5.80f, 900.0f},
};
const int NUM_HIST = 5;
int histSel = 0;

// ── Configurações ────────────────────────────────────────────
int  confSel     = 0;
bool confEditando = false;

// ── Controle de botões ───────────────────────────────────────
unsigned long ultimoNav  = 0;
unsigned long ultimoConf = 0;
unsigned long ultimoVolt = 0;
#define BTN_INTERVALO 200

// ── Utilitários de desenho ────────────────────────────────────
void desenharHeader(const char* titulo, const char* info = nullptr) {
  tft.fillRect(0, 0, W, 22, COR_HEADER);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(1);
  tft.setCursor(6, 7);
  tft.print(titulo);
  if (info) {
    tft.setCursor(W - strlen(info)*6 - 6, 7);
    tft.print(info);
  }
}

void desenharBotao(int x, int y, int w, int h, const char* texto, uint16_t cor) {
  tft.fillRoundRect(x, y, w, h, 4, cor);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(1);
  int tw = strlen(texto) * 6;
  tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
  tft.print(texto);
}

void desenharRodape(const char* btnEsq, uint16_t corEsq, const char* btnDir, uint16_t corDir) {
  int y = H - 26;
  tft.fillRect(0, y - 4, W, 4, COR_CINZA_ESC);
  desenharBotao(4,   y, 150, 22, btnEsq, corEsq);
  desenharBotao(162, y, 154, 22, btnDir, corDir);
}

void sepH(int y, uint16_t cor = COR_CINZA_ESC) {
  tft.drawFastHLine(0, y, W, cor);
}

// ── TELA 1: Monitoramento ─────────────────────────────────────
void desenharTela1() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Monitor Qualidade Agua", "*");

  int qw = W / 2;
  int qh = 70;
  int qy = 24;

  bool phOk  = (sim_ph  >= lim_ph_min && sim_ph  <= lim_ph_max);
  bool tdsOk = (sim_tds <= lim_tds);

  tft.fillRect(0, qy, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, qy + 5); tft.print("pH");
  tft.setTextColor(phOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(6, qy + 18);
  char buf[16]; dtostrf(sim_ph, 4, 2, buf); tft.print(buf);

  tft.fillRect(qw + 1, qy, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(qw + 6, qy + 5); tft.print("TDS");
  tft.setTextColor(tdsOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(qw + 6, qy + 18);
  dtostrf(sim_tds, 6, 0, buf); tft.print(buf);
  tft.setTextSize(1); tft.print(" ppm");

  sepH(qy + qh);
  int qy2 = qy + qh + 2;

  bool turbOk = (sim_turb <= lim_turb);
  tft.fillRect(0, qy2, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, qy2 + 5); tft.print("TURBIDEZ");
  tft.setTextColor(turbOk ? COR_VERDE : COR_VERMELHO); tft.setTextSize(2);
  tft.setCursor(6, qy2 + 18);
  dtostrf(sim_turb, 4, 1, buf); tft.print(buf);
  tft.setTextSize(1); tft.print(" NTU");

  tft.fillRect(qw + 1, qy2, qw - 1, qh, COR_CINZA_ESC);
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(qw + 6, qy2 + 5); tft.print("TEMP");
  tft.setTextColor(COR_BRANCO); tft.setTextSize(2);
  tft.setCursor(qw + 6, qy2 + 18);
  dtostrf(sim_temp, 4, 1, buf); tft.print(buf);
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
    dtostrf(lim_ph_min,3,1,buf); tft.print(buf);
    tft.print(" - ");
    dtostrf(lim_ph_max,3,1,buf); tft.print(buf);
    tft.print(")");
  }

  desenharRodape("  Registrar", COR_BTN_CINZA, "  Menu >", COR_BTN_VERDE);
}

// ── TELA 2: Menu principal ────────────────────────────────────
void desenharTela2() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Menu");

  for (int i = 0; i < NUM_MENU; i++) {
    int y = 28 + i * 36;
    bool sel = (i == menuSel);
    tft.fillRect(2, y, W - 4, 32, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 8);
    tft.print(sel ? "> " : "  "); tft.print(itensMenu[i]);
    tft.setCursor(W - 16, y + 8); tft.print(">");
    sepH(y + 32);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Entrar >", COR_BTN_VERDE);
}

// ── TELA 3A: Submenu Calibração ───────────────────────────────
void desenharTela3Menu() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Calibracao — Selecione sensor");

  for (int i = 0; i < NUM_CALIB; i++) {
    int y = 30 + i * 50;
    bool sel = (i == calibSel);
    tft.fillRect(2, y, W - 4, 44, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 14);
    tft.print(sel ? "> " : "  "); tft.print(itensCalib[i]);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Iniciar >", COR_BTN_VERDE);
}

// ── TELA 3B: Fluxo de calibração ─────────────────────────────
void desenharTela3Fluxo() {
  tft.fillScreen(COR_FUNDO);

  char hdr[32];
  int total = getTotalPassos();
  snprintf(hdr, sizeof(hdr), "Calibracao - %s  %d/%d",
           itensCalib[calibSel], calibPasso + 1, total);
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
    tft.print("Passo "); tft.print(calibPasso + 1); tft.print(" de 3 — Solucao tampao ");
    tft.print(passosCalibPH[calibPasso]);
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe o eletrodo e aguarde...");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    float vSim = 2.5f - (calibPasso == 1 ? 0.688f : (calibPasso == 2 ? -0.688f : 0.0f));
    char vbuf[16]; dtostrf(vSim, 5, 3, vbuf);
    tft.print("V = "); tft.print(vbuf); tft.print(" V  ok");
  } else if (calibSel == 1) {
    tft.print("Passo 1 de 1 — Solucao padrao 1413 uS/cm");
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe a sonda e o DS18B20.");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    tft.print("V = 1.413 V  ok");
  } else {
    tft.print("Passo "); tft.print(calibPasso + 1); tft.print(" de 3 — ");
    tft.print(passosCalibTurb[calibPasso]);
    tft.setCursor(6, iy + 14); tft.setTextColor(COR_BRANCO);
    tft.print("Mergulhe o sensor e aguarde...");
    tft.setCursor(6, iy + 32); tft.setTextColor(COR_VERDE); tft.setTextSize(2);
    float vSim = 3.8f - calibPasso * 0.9f;
    char vbuf[16]; dtostrf(vSim, 5, 3, vbuf);
    tft.print("V = "); tft.print(vbuf); tft.print(" V  ok");
  }

  tft.setTextSize(1);
  if (calibPasso < total - 1) {
    tft.setTextColor(COR_CINZA); tft.setCursor(6, iy + 68);
    if (calibSel == 0) { tft.print("Proximo — "); tft.print(passosCalibPH[calibPasso + 1]); }
    else if (calibSel == 2) { tft.print("Proximo — "); tft.print(passosCalibTurb[calibPasso + 1]); }
    tft.setCursor(6, iy + 80); tft.setTextColor(COR_AMARELO);
    tft.print("Aguardando confirmacao...");
  } else {
    tft.setTextColor(COR_AMARELO); tft.setCursor(6, iy + 68);
    tft.print("Ultimo passo — Confirmar para salvar NVS");
  }

  const char* lblDir = (calibPasso < total - 1) ? "Proximo >" : "Salvar >";
  desenharRodape("X Cancelar", COR_BTN_CINZA, lblDir, COR_BTN_VERDE);
}

// ── TELA 4: Registro de amostra ───────────────────────────────
void desenharTela4() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Registrar Amostra");
  tft.setTextColor(COR_CINZA); tft.setTextSize(1);
  tft.setCursor(6, 28); tft.print("Selecione o rotulo:");

  for (int i = 0; i < NUM_ROTULOS; i++) {
    int y = 40 + i * 30;
    bool sel = (i == rotuloSel);
    tft.fillRect(2, y, W - 4, 26, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(12, y + 9);
    tft.print(sel ? "> " : "  "); tft.print(rotulos[i]);
  }

  desenharRodape("X Cancelar", COR_BTN_CINZA, "Salvar >", COR_BTN_VERDE);
}

// ── TELA 5: Histórico ─────────────────────────────────────────
void desenharTela5() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Historico — agua_log.csv");

  for (int i = 0; i < NUM_HIST; i++) {
    int y = 26 + i * 36;
    bool sel = (i == histSel);
    tft.fillRect(2, y, W - 4, 32, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    bool phOk = (historico[i].ph >= lim_ph_min && historico[i].ph <= lim_ph_max);

    tft.setTextColor(COR_CINZA); tft.setTextSize(1);
    tft.setCursor(6, y + 6);
    tft.print(historico[i].hora); tft.print(" | ");
    tft.setTextColor(COR_BRANCO); tft.print(historico[i].rotulo); tft.print(" | pH ");
    char buf[8]; dtostrf(historico[i].ph, 4, 2, buf);
    tft.setTextColor(phOk ? COR_VERDE : COR_VERMELHO); tft.print(buf);
    tft.setTextColor(COR_CINZA); tft.print("  ");
    dtostrf(historico[i].tds, 5, 0, buf); tft.print(buf); tft.print("ppm");
    sepH(y + 32);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Ver >", COR_BTN_VERDE);
}

// ── TELA 6: Configurações ─────────────────────────────────────
void desenharTela6() {
  tft.fillScreen(COR_FUNDO);
  desenharHeader("Configuracoes");

  const char* nomes[] = {
    "Limite pH min", "Limite pH max", "Limite TDS",
    "Limite turbidez", "Intervalo leitura", "Arquivo log"
  };
  const int NUM_CONF = 6;

  for (int i = 0; i < NUM_CONF; i++) {
    int y = 26 + i * 32;
    bool sel  = (i == confSel);
    bool edit = sel && confEditando;
    tft.fillRect(2, y, W - 4, 28, sel ? COR_SELECIONADO : COR_CINZA_ESC);
    tft.setTextColor(COR_BRANCO); tft.setTextSize(1);
    tft.setCursor(8, y + 10); tft.print(nomes[i]);
    char vbuf[24];
    tft.setTextColor(edit ? COR_AMARELO : (sel ? COR_VERDE : COR_CIANO));
    tft.setCursor(200, y + 10);
    if (edit) tft.print("< ");
    switch(i) {
      case 0: dtostrf(lim_ph_min, 4, 1, vbuf); tft.print(vbuf); break;
      case 1: dtostrf(lim_ph_max, 4, 1, vbuf); tft.print(vbuf); break;
      case 2: snprintf(vbuf, sizeof(vbuf), "%d ppm", (int)lim_tds); tft.print(vbuf); break;
      case 3: snprintf(vbuf, sizeof(vbuf), "%d NTU", (int)lim_turb); tft.print(vbuf); break;
      case 4: snprintf(vbuf, sizeof(vbuf), "%d s", lim_interv); tft.print(vbuf); break;
      case 5: tft.print("agua_log"); break;
    }
    if (edit) tft.print(" >");
    sepH(y + 28);
  }

  desenharRodape("< Voltar", COR_BTN_CINZA, "Salvar >", COR_BTN_VERDE);
}

// ── Despacho de telas ─────────────────────────────────────────
void desenharTela() {
  switch (estadoAtual) {
    case MONITORAMENTO:    desenharTela1();      break;
    case MENU_PRINCIPAL:   desenharTela2();      break;
    case CALIBRACAO_MENU:  desenharTela3Menu();  break;
    case CALIBRACAO_FLUXO: desenharTela3Fluxo(); break;
    case REGISTRO:         desenharTela4();      break;
    case HISTORICO:        desenharTela5();      break;
    case CONFIGURACOES:    desenharTela6();      break;
  }
}

// ── Ações dos botões ─────────────────────────────────────────
void acaoNavegar() {
  switch (estadoAtual) {
    case MENU_PRINCIPAL:   menuSel   = (menuSel   + 1) % NUM_MENU;   break;
    case CALIBRACAO_MENU:  calibSel  = (calibSel  + 1) % NUM_CALIB;  break;
    case REGISTRO:         rotuloSel = (rotuloSel + 1) % NUM_ROTULOS; break;
    case HISTORICO:        histSel   = (histSel   + 1) % NUM_HIST;   break;
    case CONFIGURACOES:
      if (confEditando) {
        switch (confSel) {
          case 0: lim_ph_min = min(lim_ph_min + 0.1f, lim_ph_max - 0.1f); break;
          case 1: lim_ph_max = min(lim_ph_max + 0.1f, 14.0f);             break;
          case 2: lim_tds    = min(lim_tds  + 50.0f, 5000.0f);            break;
          case 3: lim_turb   = min(lim_turb + 10.0f, 1000.0f);            break;
          case 4: lim_interv = min(lim_interv + 1, 60);                   break;
        }
      } else {
        confSel = (confSel + 1) % 6;
      }
      break;
    default: break;
  }
  desenharTela();
}

void acaoConfirmar() {
  switch (estadoAtual) {
    case MONITORAMENTO:
      estadoAtual = MENU_PRINCIPAL; menuSel = 0;
      break;

    case MENU_PRINCIPAL:
      switch (menuSel) {
        case 0: estadoAtual = CALIBRACAO_MENU; calibSel = 0;  break;
        case 1: estadoAtual = HISTORICO;       histSel  = 0;  break;
        case 2: estadoAtual = CONFIGURACOES;   confSel  = 0; confEditando = false; break;
        case 3: estadoAtual = REGISTRO;        rotuloSel = 0; break;
      }
      break;

    case CALIBRACAO_MENU:
      estadoAtual = CALIBRACAO_FLUXO; calibPasso = 0;
      break;

    case CALIBRACAO_FLUXO: {
      int total = getTotalPassos();
      if (calibPasso < total - 1) {
        calibPasso++;
      } else {
        tft.fillScreen(COR_FUNDO);
        desenharHeader("Calibracao");
        tft.setTextColor(COR_VERDE); tft.setTextSize(2);
        tft.setCursor(60, 90); tft.print("SALVO  OK!");
        tft.setTextSize(1); tft.setTextColor(COR_BRANCO);
        tft.setCursor(40, 120); tft.print("Coeficientes gravados na NVS.");
        delay(2000);
        estadoAtual = CALIBRACAO_MENU;
      }
      break;
    }

    case REGISTRO: {
      tft.fillScreen(COR_FUNDO);
      desenharHeader("Registro");
      tft.setTextColor(COR_VERDE); tft.setTextSize(2);
      tft.setCursor(40, 80); tft.print("SALVO NO SD!");
      tft.setTextSize(1); tft.setTextColor(COR_BRANCO);
      tft.setCursor(20, 110); tft.print("Rotulo: "); tft.println(rotulos[rotuloSel]);
      tft.setCursor(20, 126);
      char linha[64];
      snprintf(linha, sizeof(linha), "pH:%.2f TDS:%.0fppm Turb:%.1fNTU T:%.1fC",
               sim_ph, sim_tds, sim_turb, sim_temp);
      tft.print(linha);
      delay(2500);
      estadoAtual = MENU_PRINCIPAL;
      break;
    }

    case HISTORICO: {
      tft.fillScreen(COR_FUNDO);
      desenharHeader("Detalhe do registro");
      int r = histSel;
      tft.setTextColor(COR_CIANO); tft.setTextSize(1);
      tft.setCursor(6, 30); tft.print("Data/Hora: "); tft.setTextColor(COR_BRANCO); tft.print(historico[r].hora);
      tft.setTextColor(COR_CIANO);
      tft.setCursor(6, 48); tft.print("Rotulo:    "); tft.setTextColor(COR_BRANCO); tft.print(historico[r].rotulo);
      tft.setTextColor(COR_CIANO);
      tft.setCursor(6, 66); tft.print("pH:        ");
      bool ok = (historico[r].ph >= lim_ph_min && historico[r].ph <= lim_ph_max);
      tft.setTextColor(ok ? COR_VERDE : COR_VERMELHO); tft.print(historico[r].ph, 2);
      tft.setTextColor(COR_CIANO);
      tft.setCursor(6, 84); tft.print("TDS:       "); tft.setTextColor(COR_BRANCO); tft.print(historico[r].tds, 0); tft.print(" ppm");
      tft.setTextColor(COR_AMARELO);
      tft.setCursor(6, 110); tft.print("Pressione VOLTAR para retornar.");
      while (digitalRead(BTN_VOLTAR) == HIGH) { delay(10); }
      delay(DEBOUNCE_MS);
      break;
    }

    case CONFIGURACOES:
      if (confSel == 5) break;
      confEditando = !confEditando;
      break;
  }
  desenharTela();
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
  desenharTela();
}

// ── Leitura de botão com debounce e anti-repetição ───────────
bool lerBotao(int pino, unsigned long& ultimoMs) {
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

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(BTN_NAVEGAR,   INPUT_PULLUP);
  pinMode(BTN_CONFIRMAR, INPUT_PULLUP);
  pinMode(BTN_VOLTAR,    INPUT_PULLUP);

  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(COR_FUNDO);

  tft.setTextColor(COR_CIANO); tft.setTextSize(2);
  tft.setCursor(30, 80);  tft.print("Monitor Qualidade");
  tft.setCursor(60, 104); tft.print("Agua — UFMT");
  tft.setTextSize(1); tft.setTextColor(COR_CINZA);
  tft.setCursor(60, 140); tft.print("Teste IHM — dados simulados");
  delay(2000);

  desenharTela();
  Serial.println("IHM pronta. Botoes: NAV=32 CONF=33 VOLT=25");
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  if (lerBotao(BTN_NAVEGAR,   ultimoNav))  { Serial.println("BTN: NAVEGAR");   acaoNavegar(); }
  if (lerBotao(BTN_CONFIRMAR, ultimoConf)) { Serial.println("BTN: CONFIRMAR"); acaoConfirmar(); }
  if (lerBotao(BTN_VOLTAR,    ultimoVolt)) { Serial.println("BTN: VOLTAR");    acaoVoltar(); }
}
```

---

## 7. Diagnóstico de falhas

| Sintoma | Causa provável | Solução |
|---|---|---|
| Tela em branco após boot | TFT não inicializou | Verificar CS=5, DC=2, RST=4 e alimentação 3,3V |
| Tela de cabeça para baixo | Rotação errada | Trocar `setRotation(1)` por `setRotation(3)` |
| Botão não responde | GPIO errado ou pullup ausente | Verificar `pinMode(pino, INPUT_PULLUP)` e ligação GPIO → GND |
| Botão dispara múltiplas vezes | Debounce insuficiente | Aumentar `DEBOUNCE_MS` de 20 para 50 ms |
| Navegação circular não funciona | Operador `%` com valor negativo | Usar `(sel + 1) % N` — incremento positivo garante comportamento correto |
| Serial Monitor não exibe nada | Baud rate errado | Configurar para 115200 baud |
