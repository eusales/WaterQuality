# Relatório Técnico — Firmware Monitor Portátil de Qualidade da Água

**Projeto:** Monitor Portátil de Qualidade da Água  
**Disciplina:** Instrumentação Industrial II — UFMT  
**Plataforma:** ESP32 DevKit V3 (WROOM-32)  
**Framework:** Arduino  
**IDE:** PlatformIO (VS Code)

---

## 1. Introdução

### 1.1 Objetivo

Desenvolver o firmware MVP de um monitor portátil de qualidade da água capaz de:

- Ler quatro parâmetros em tempo real: **pH**, **TDS** (ppm), **Turbidez** (NTU) e **Temperatura** (°C)
- Exibir resultados em display TFT colorido de 2,0 polegadas
- Registrar dados em cartão SD no formato CSV
- Permitir calibração de sensores com persistência em NVS flash
- Emitir alertas sonoros quando parâmetros ultrapassam limites configuráveis

### 1.2 Escopo (Fase 1)

O dispositivo opera **totalmente offline**. O servidor web WiFi é preparado arquiteturalmente mas desativado (`FASE2_WEB 0`).

---

## 2. Arquitetura de Hardware

### 2.1 Componentes

| Componente | Modelo | Interface | Observação |
|---|---|---|---|
| Microcontrolador | ESP32 DevKit V3 (WROOM-32) | — | Lógica 3,3V |
| ADC externo | ADS1115 16-bit | I²C @ 0x48 | Alimentado em 5V |
| Sensor de pH | PH4502C | Analógico → ADS1115 AIN2 | Precisa de 5V |
| Sensor de TDS | Gravity Analog TDS | Analógico → ADS1115 AIN1 | Saída máx 2,3V |
| Sensor de turbidez | TS-300B | Analógico → ADS1115 AIN0 | Alimentado em 5V |
| Sensor de temperatura | DS18B20 | 1-Wire (GPIO 13) | Pull-up 4,7kΩ |
| Display | TFT ST7789 2,0" (320×240) | SPI compartilhado | 3,3V |
| Leitor SD | Micro SD SPI | SPI compartilhado | 3,3V, FAT32 |
| Conversor de nível | TXS0108E (4 canais) | I²C digital | 3,3V ↔ 5V |
| Buzzer | Buzzer ativo | GPIO 26 | OUTPUT |
| Botões | 3 chipes tácteis | GPIO 32/33/25 | INPUT_PULLUP → GND |

### 2.2 Pinout Final Validado

```
ESP32 DevKit V3
┌────────────────────────────────────────────┐
│  GPIO 5  ──── TFT CS                       │
│  GPIO 17 ──── TFT DC  (NÃO GPIO 2!)       │
│  GPIO 4  ──── TFT RST                      │
│  GPIO 15 ──── SD CS                        │
│  GPIO 18 ──── SCK (TFT + SD)              │
│  GPIO 23 ──── MOSI (TFT + SD)             │
│  GPIO 19 ──── MISO (SD apenas)            │
│  GPIO 21 ──── SDA → TXS0108E → ADS1115   │
│  GPIO 22 ──── SCL → TXS0108E → ADS1115   │
│  GPIO 13 ──── DS18B20 DATA (1-Wire)       │
│  GPIO 32 ──── Botão Navegar (→ GND)       │
│  GPIO 33 ──── Botão Confirmar (→ GND)     │
│  GPIO 25 ──── Botão Voltar (→ GND)        │
│  GPIO 26 ──── Buzzer ativo (→ GND)        │
└────────────────────────────────────────────┘
```

### 2.3 Decisão Crítica: ADS1115 em 5V

```
Sensor (5V) ──analógico──▶ ADS1115 (VDD=5V) ──I²C 5V──▶ TXS0108E ──I²C 3.3V──▶ ESP32
```

- O ADS1115 alimentado em 5V tolera entradas analógicas até ~5,5V — resolve o problema de tensão sem divisores de tensão
- O TXS0108E é **somente digital** — fica exclusivamente no barramento I²C, convertendo sinais 5V↔3,3V
- **Nunca** colocar o TXS0108E na cadeia analógica (distorce o sinal)

### 2.4 Mapa de Canais ADS1115

| Canal | Sensor | Faixa de tensão | Grandeza |
|---|---|---|---|
| AIN0 | Turbidez TS-300B | 0 – 3,3V | NTU |
| AIN1 | TDS Gravity | 0 – 2,3V | ppm |
| AIN2 | pH PH4502C | 1,0 – 4,0V | pH |
| AIN3 | Reserva | — | Expansão futura |

---

## 3. Arquitetura de Software

### 3.1 Estrutura de Diretórios (PlatformIO)

```
monitor_agua/
├── platformio.ini              # Configuração de build
├── include/
│   └── config.h                # GPIOs, structs, enums, constantes
└── src/
    ├── main.cpp                # Setup + Loop (43 linhas)
    ├── sensores.h / .cpp       # Leitura ADS1115 + DS18B20 + cálculo
    ├── display.h  / .cpp       # 6 telas TFT + splash
    ├── botoes.h   / .cpp       # Debounce + despacho para máquina
    ├── maquina.h  / .cpp       # Máquina de estados (7 estados)
    ├── nvs_cal.h  / .cpp       # Persistência NVS + backup JSON
    ├── sd_log.h   / .cpp       # Log CSV em cartão SD
    ├── buzzer.h   / .cpp       # 3 padrões sonoros
    └── web.h      / .cpp       # Servidor HTTP (Fase 2)
```

### 3.2 Grafo de Dependências

```
config.h  ◄───────────── todos os módulos

sensores ──────────────► (sem deps internas)
display  ──────────────► maquina.h (enum Estado)
botoes   ──────────────► maquina.h
maquina  ──────────────► display.h, nvs_cal.h, sd_log.h, buzzer.h
nvs_cal  ──────────────► sd_log.h
sd_log   ──────────────► (sem deps internas)
buzzer   ──────────────► (sem deps internas)
web      ──────────────► sensores.h, sd_log.h (só Fase 2)
main     ──────────────► todos
```

**Regra absoluta:** Apenas `maquina.cpp` importa `display.h`. Apenas `botoes.cpp` e `main.cpp` importa `maquina.h`.

### 3.3 Princípios de Design

| Princípio | Implementação |
|---|---|
| Responsabilidade única | Cada módulo tem uma função bem definida |
| Baixo acoplamento | Módulos se comunicam via structs `extern` e funções públicas |
| Alto coesão | Código relacionado está agrupado no mesmo arquivo |
| Separation of concerns | Display não lê sensor, botões não desenham, sensores não sabem de IHM |

---

## 4. Análise dos Módulos

### 4.1 `config.h` — Configuração Global

**Responsabilidade:** Único arquivo alterado em mudanças de hardware.

```cpp
// ── GPIOs TFT ST7789 (SPI compartilhado) ─────────────────────
#define TFT_CS    5
#define TFT_DC   17
#define TFT_RST   4

// ── GPIOs SD Card (SPI compartilhado) ────────────────────────
#define SD_CS    15

// ── GPIOs I2C (ADS1115 via TXS0108E) ────────────────────────
#define SDA_PIN  21
#define SCL_PIN  22

// ── GPIO 1-Wire (DS18B20) ───────────────────────────────────
#define ONE_WIRE  34

// ── GPIOs Botões (INPUT_PULLUP) ─────────────────────────────
#define BTN_NAV   32
#define BTN_CONF  33
#define BTN_VOLT  25

// ── GPIO Buzzer ─────────────────────────────────────────────
#define BUZZER    26

// ── ADS1115 ─────────────────────────────────────────────────
#define ADS1115_ADDR   0x48
#define ADS1115_GAIN   GAIN_ONE

// ── Limites padrão de alerta ────────────────────────────────
#define PH_MIN_DEFAULT     6.0f
#define PH_MAX_DEFAULT     9.0f
#define TDS_LIMIT_DEFAULT  1000.0f
#define TURB_LIMIT_DEFAULT 100.0f

// ── Arquivos ────────────────────────────────────────────────
#define CSV_FILENAME       "/agua_log.csv"
#define CAL_BACKUP_FILE    "/cal_backup.json"

// ── Fase 2: Web (0 = desativado, 1 = ativo) ────────────────
#define FASE2_WEB 0

// ── Intervalo de leitura padrão (ms) ────────────────────────
#define DEFAULT_INTERVAL_MS 2000

// ── Struct de leituras compartilhada ────────────────────────
struct Leituras {
  float ph;
  float tds_ppm;
  float turbidez_ntu;
  float temp_c;
  bool  ph_ok;
  bool  tds_ok;
  bool  turb_ok;
};

extern Leituras dados;

// ── Struct de calibração ────────────────────────────────────
struct CalCoef {
  float ph_offset;
  float ph_slope_acid;
  float ph_slope_bas;
  float k_tds;
  float turb_a;
  float turb_b;
  float turb_c;
};

extern CalCoef calib;

// ── Estados da máquina de estados ────────────────────────────
enum Estado {
  MONITORAMENTO,
  MENU_PRINCIPAL,
  CALIBRACAO_MENU,
  CALIBRACAO_FLUXO,
  REGISTRO,
  HISTORICO,
  CONFIGURACOES
};
```

---

### 4.2 `sensores.cpp` — Aquisição e Cálculo

**Responsabilidade:** Ler hardware analógico (ADS1115) e temperatura (DS18B20), aplicar calibração, preencher struct `dados`.

```cpp
static Adafruit_ADS1X15 ads;
static OneWire oneWire(ONE_WIRE);
static DallasTemperature sensors(&oneWire);

Leituras dados;
CalCoef  calib = {0, 1, 1, 1, 0, 0, 0};

void inicializarSensores() {
  Wire.begin(SDA_PIN, SCL_PIN);
  ads.begin(ADS1115_ADDR);
  ads.setGain(ADS1115_GAIN);
  sensors.begin();
}

float calcPH(float v) {
  if (v < calib.ph_offset) {
    return calib.ph_slope_acid * (v - calib.ph_offset) + 7.0f;
  } else {
    return calib.ph_slope_bas * (v - calib.ph_offset) + 7.0f;
  }
}

float calcTDS(float v, float temp) {
  float ec_bruto = v * 1000.0f / 2.3f;
  float ec_corr  = ec_bruto * calib.k_tds / (1.0f + 0.02f * (temp - 25.0f));
  return ec_corr / 2.0f;
}

float calcNTU(float v) {
  return calib.turb_a * v * v + calib.turb_b * v + calib.turb_c;
}

void verificarAlertas() {
  dados.ph_ok   = (dados.ph >= PH_MIN_DEFAULT && dados.ph <= PH_MAX_DEFAULT);
  dados.tds_ok  = (dados.tds_ppm <= TDS_LIMIT_DEFAULT);
  dados.turb_ok = (dados.turbidez_ntu <= TURB_LIMIT_DEFAULT);
}

void lerTodos() {
  sensors.requestTemperatures();
  dados.temp_c = sensors.getTempCByIndex(0);

  int16_t raw_ph   = ads.readADC_SingleEnded(2);
  int16_t raw_tds  = ads.readADC_SingleEnded(1);
  int16_t raw_turb = ads.readADC_SingleEnded(0);

  float v_ph   = ads.computeVolts(raw_ph);
  float v_tds  = ads.computeVolts(raw_tds);
  float v_turb = ads.computeVolts(raw_turb);

  dados.ph           = calcPH(v_ph);
  dados.tds_ppm      = calcTDS(v_tds, dados.temp_c);
  dados.turbidez_ntu = calcNTU(v_turb);

  verificarAlertas();

  Serial.printf("[LEITURA] pH:%.2f TDS:%.1fppm Turb:%.1fNTU Temp:%.1fC\n",
                dados.ph, dados.tds_ppm, dados.turbidez_ntu, dados.temp_c);
}
```

---

### 4.3 `display.cpp` — Renderização das 6 Telas

**Responsabilidade:** Desenhar telas no TFT. Recebe estado atual e struct `dados`. Não lê sensor.

```cpp
static Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void inicializarDisplay() {
  tft.init(240, 320);
  tft.setRotation(3);
  tft.fillScreen(COR_FUNDO);
}

void desenharSplash() {
  tft.fillScreen(COR_FUNDO);
  tft.setTextColor(COR_BRANCO);
  tft.setTextSize(2);
  tft.setCursor(40, 80);  tft.print("Monitor de");
  tft.setCursor(60, 110); tft.print("Qualidade");
  tft.setCursor(90, 140); tft.print("da Agua");
  tft.setTextSize(1);
  tft.setTextColor(COR_CINZA);
  tft.setCursor(100, 180); tft.print("Inicializando...");
}

// Despacho para as 6 telas
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
```

**Tela 1 — Monitoramento:** Layout 2×2 com pH, TDS, Turbidez, Temperatura. Cores verde/vermelho conforme limites.

**Tela 2 — Menu Principal:** Lista de 4 itens com navegação circular.

**Tela 3 — Calibração:** Submenu de seleção de sensor + fluxo passo a passo com barra de progresso.

**Tela 4 — Registro:** Seleção de rótulo (5 opções) para gravação no SD.

**Tela 5 — Histórico:** Registros paginados do CSV (5 por página).

**Tela 6 — Configurações:** 6 parâmetros editáveis inline com `< valor >`.

---

### 4.4 `maquina.cpp` — Máquina de Estados

**Responsabilidade:** Cérebro da IHM. Contém enum Estado, variáveis de seleção, fluxo de calibração e as 3 ações de navegação.

```cpp
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

// Variáveis temporárias de calibração
static float calibV7 = 0, calibV4 = 0, calibV10 = 0;
static float calibV0 = 0, calibV50 = 0, calibV500 = 0;
```

**Fluxo de calibração (`processarCalibracao`):**

```cpp
static void processarCalibracao() {
  int total = (calibSel == 1) ? 1 : 3;
  float vAtual = dados.ph;

  // Captura tensão do passo atual
  if (calibSel == 0) {                          // pH
    if (calibPasso == 0) calibV7  = vAtual;
    else if (calibPasso == 1) calibV4  = vAtual;
    else if (calibPasso == 2) calibV10 = vAtual;
  } else if (calibSel == 2) {                   // Turbidez
    if (calibPasso == 0) calibV0   = vAtual;
    else if (calibPasso == 1) calibV50  = vAtual;
    else if (calibPasso == 2) calibV500 = vAtual;
  }

  if (calibPasso < total - 1) {
    calibPasso++;  // Avança para próximo passo
  } else {
    // Último passo — calcula e salva
    if (calibSel == 0) {                         // pH
      calib.ph_offset    = calibV7;
      calib.ph_slope_acid = (7.0f - 4.0f) / (calibV7 - calibV4);
      calib.ph_slope_bas  = (7.0f - 10.0f) / (calibV7 - calibV10);
    } else if (calibSel == 1) {                  // TDS
      float ec25 = (vAtual * 1000.0f / 2.3f) / (1.0f + 0.02f * (dados.temp_c - 25.0f));
      calib.k_tds = 1413.0f / ec25;
    } else if (calibSel == 2) {                  // Turbidez
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
```

---

### 4.5 `botoes.cpp` — Entrada com Debounce

**Responsabilidade:** Leitura física dos 3 botões com debounce por software e anti-repetição.

```cpp
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
```

**Lógica de debounce:**
1. Detecta `LOW` no GPIO (botão pressionado)
2. Aguarda 20ms (debounce mecânico)
3. Verifica se ainda está `LOW`
4. Verifica intervalo mínimo de 200ms desde a última pressão
5. Retorna `true` apenas se todas as condições forem atendidas

---

### 4.6 `nvs_cal.cpp` — Persistência NVS + Backup SD

**Responsabilidade:** Salvar e carregar coeficientes de calibração. NVS é primário; JSON no SD é backup.

```cpp
void carregarCalibracao() {
  prefs.begin("calibracao", true);

  if (prefs.isKey("ph_offset")) {
    // Carrega da NVS (primário)
    calib.ph_offset     = prefs.getFloat("ph_offset",     0);
    calib.ph_slope_acid = prefs.getFloat("ph_slope_acid", 1);
    calib.ph_slope_bas  = prefs.getFloat("ph_slope_bas",  1);
    calib.k_tds         = prefs.getFloat("k_tds",         1);
    calib.turb_a        = prefs.getFloat("turb_a",        0);
    calib.turb_b        = prefs.getFloat("turb_b",        0);
    calib.turb_c        = prefs.getFloat("turb_c",        0);
  } else {
    // Fallback: restaura do backup SD
    File f = SD.open(CAL_BACKUP_FILE, FILE_READ);
    if (f) {
      JsonDocument doc;
      deserializeJson(doc, f);
      f.close();
      calib.ph_offset     = doc["ph_offset"]     | 0.0f;
      calib.ph_slope_acid = doc["ph_slope_acid"] | 1.0f;
      calib.ph_slope_bas  = doc["ph_slope_bas"]  | 1.0f;
      calib.k_tds         = doc["k_tds"]         | 1.0f;
      calib.turb_a        = doc["turb_a"]        | 0.0f;
      calib.turb_b        = doc["turb_b"]        | 0.0f;
      calib.turb_c        = doc["turb_c"]        | 0.0f;
    }
    // Se nada encontrado → valores padrão
  }
  prefs.end();
}

void salvarCalibracao() {
  // Salva na NVS
  prefs.begin("calibracao", false);
  prefs.putFloat("ph_offset",     calib.ph_offset);
  prefs.putFloat("ph_slope_acid", calib.ph_slope_acid);
  prefs.putFloat("ph_slope_bas",  calib.ph_slope_bas);
  prefs.putFloat("k_tds",         calib.k_tds);
  prefs.putFloat("turb_a",        calib.turb_a);
  prefs.putFloat("turb_b",        calib.turb_b);
  prefs.putFloat("turb_c",        calib.turb_c);
  prefs.end();

  // Salva backup no SD
  File f = SD.open(CAL_BACKUP_FILE, FILE_WRITE);
  if (f) {
    JsonDocument doc;
    doc["ph_offset"]     = calib.ph_offset;
    doc["ph_slope_acid"] = calib.ph_slope_acid;
    doc["ph_slope_bas"]  = calib.ph_slope_bas;
    doc["k_tds"]         = calib.k_tds;
    doc["turb_a"]        = calib.turb_a;
    doc["turb_b"]        = calib.turb_b;
    doc["turb_c"]        = calib.turb_c;
    serializeJson(doc, f);
    f.close();
  }
}
```

**Fluxo de boot:**
```
NVS (Preferences) → encontrado? → usa valores da NVS
                   → não encontrado? → tenta cal_backup.json no SD
                                      → encontrado? → restaura do SD
                                      → não encontrado? → usa defaults (0,1,1,1,0,0,0)
```

---

### 4.7 `sd_log.cpp` — Log CSV

**Responsabilidade:** Inicializar SD, criar CSV com cabeçalho, gravar linhas, ler histórico paginado.

```cpp
void inicializarSD() {
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] Falha na inicializacao!");
    return;
  }
  // Cria CSV com cabeçalho se não existir
  if (!SD.exists(CSV_FILENAME)) {
    File f = SD.open(CSV_FILENAME, FILE_WRITE);
    if (f) {
      f.println("timestamp,rotulo,ph,tds_ppm,turbidez_ntu,temperatura_c");
      f.close();
    }
  }
}

void gravarLinha(const char* rotulo) {
  File f = SD.open(CSV_FILENAME, FILE_APPEND);
  if (!f) return;

  char linha[128];
  snprintf(linha, sizeof(linha), "%s,%.2f,%.1f,%.1f,%.1f",
           rotulo, dados.ph, dados.tds_ppm, dados.turbidez_ntu, dados.temp_c);
  f.println(linha);
  f.close();
}

void lerHistorico(int pagina, Registro* buf, int* total) {
  File f = SD.open(CSV_FILENAME, FILE_READ);
  if (!f) { *total = 0; return; }

  int count = 0, perPage = 5, lineNum = 0;
  int startLine = pagina * perPage + 1;

  while (f.available() && count < perPage) {
    String linha = f.readStringUntil('\n');
    linha.trim();
    if (linha.length() == 0) continue;

    if (lineNum >= startLine) {
      // Parse CSV: hora,rotulo,ph,tds,turb,temp
      // ... (parsing campo a campo)
      count++;
    }
    lineNum++;
  }
  *total = count;
  f.close();
}
```

**Formato CSV:**
```
timestamp,rotulo,ph,tds_ppm,turbidez_ntu,temperatura_c
Destilada,7.02,312.0,4.1,24.3
```

---

### 4.8 `buzzer.cpp` — Alertas Sonoros

**Responsabilidade:** 3 padrões de som no GPIO 26.

```cpp
void inicializarBuzzer() {
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void bipAlerta() {          // 1 bip curto (100ms)
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void bipConfirmacao() {     // 2 bips rápidos (80ms cada)
  digitalWrite(BUZZER, HIGH);
  delay(80);
  digitalWrite(BUZZER, LOW);
  delay(80);
  digitalWrite(BUZZER, HIGH);
  delay(80);
  digitalWrite(BUZZER, LOW);
}

void bipErro() {            // 3 bips lentos (200ms cada)
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}
```

---

### 4.9 `web.cpp` — Servidor HTTP (Fase 2)

**Responsabilidade:** Compilado apenas quando `FASE2_WEB 1`. Expõe leituras via HTTP.

```cpp
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
```

**Endpoints:**
| Método | Rota | Função |
|---|---|---|
| GET | `/leituras` | Retorna JSON com valores atuais |
| POST | `/registrar` | Grava snapshot no CSV |

---

### 4.10 `main.cpp` — Orquestrador

**Responsabilidade:** Apenas `setup()` e `loop()`. Máximo de 43 linhas.

```cpp
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
```

**Sequência de boot:**
```
1. Serial.begin(115200)
2. inicializarDisplay()      → TFT init + setRotation(3)
3. desenharSplash()          → Tela "Monitor de Qualidade da Água"
4. inicializarBotoes()       → GPIO 32/33/25 como INPUT_PULLUP
5. inicializarSensores()     → Wire, ADS1115, DS18B20
6. carregarCalibracao()      → NVS → SD backup → defaults
7. inicializarSD()           → SD.begin, criar CSV se necessário
8. inicializarBuzzer()       → GPIO 26 como OUTPUT
9. desenharTela(MONITORAMENTO) → Tela 1
```

**Loop principal:**
```
1. verificarBotoes()         → contínuo, com debounce
2. Se 2s desde última leitura:
   a. lerTodos()             → ADS1115 + DS18B20 → struct dados
   b. Se estado == MONITORAMENTO → redesenha Tela 1
3. servirWeb()               → apenas se FASE2_WEB=1
```

---

## 5. Máquina de Estados

### 5.1 Enum

```cpp
enum Estado {
  MONITORAMENTO,     // Tela 1 — tela principal
  MENU_PRINCIPAL,    // Tela 2 — 4 itens
  CALIBRACAO_MENU,   // Tela 3A — seleção de sensor
  CALIBRACAO_FLUXO,  // Tela 3B — passo a passo
  REGISTRO,          // Tela 4 — seleção de rótulo
  HISTORICO,         // Tela 5 — CSV paginado
  CONFIGURACOES      // Tela 6 — edição de parâmetros
};
```

### 5.2 Tabela de Transições

| Estado Atual | Navegar | Confirmar | Voltar |
|---|---|---|---|
| MONITORAMENTO | sem efeito | → MENU_PRINCIPAL | sem efeito |
| MENU_PRINCIPAL | percorre 4 itens (circular) | → item selecionado | → MONITORAMENTO |
| CALIBRACAO_MENU | percorre 3 sensores | → CALIBRACAO_FLUXO | → MENU_PRINCIPAL |
| CALIBRACAO_FLUXO | sem efeito | avança passo / salva NVS | → CALIBRACAO_MENU |
| REGISTRO | percorre 5 rótulos | grava CSV → MENU_PRINCIPAL | → MENU_PRINCIPAL |
| HISTORICO | percorre registros | exibe detalhe | → MENU_PRINCIPAL |
| CONFIGURACOES | seleciona / incrementa em edição | alterna modo edição (salva ao sair) | → MENU_PRINCIPAL |

### 5.3 Fluxo de Calibração

```
MENU_PRINCIPAL → Calibração
  → CALIBRACAO_MENU (seleciona pH/TDS/Turbidez)
    → CALIBRACAO_FLUXO
      pH: Passo 1 (V7) → Passo 2 (V4) → Passo 3 (V10) → calcula slopes → salva NVS
      TDS: Passo 1 (solução 1413 µS/cm) → calcula k → salva NVS
      Turbidez: Passo 1 (V0) → Passo 2 (V50) → Passo 3 (V500) → calcula a,b,c → salva NVS
```

---

## 6. Interface Humana (IHM)

### 6.1 Botões

| Botão | GPIO | Função |
|---|---|---|
| Navegar | 32 | Percorre itens (circular) |
| Confirmar | 33 | Seleciona / salva |
| Voltar | 25 | Retorna ao nível anterior |

### 6.2 Layout das Telas

**Tela 1 — Monitoramento:**
```
┌─────────────────────────────────┐
│ Monitor Qualidade Agua          │  header azul
├─────────────────┬───────────────┤
│ pH              │ TDS           │
│ 7.02            │ 1142 ppm      │  verde/vermelho
├─────────────────┼───────────────┤
│ Turbidez        │ Temp          │
│ 4.1 NTU         │ 24.3 C       │
├─────────────────┼───────────────┤
│ ! TDS acima...                 │  alerta amarelo
├─────────────────┬───────────────┤
│   Registrar     │   Menu >      │
└─────────────────┴───────────────┘
```

**Tela 2 — Menu:**
```
┌─────────────────────────────────┐
│ Menu                            │
├─────────────────────────────────┤
│ > Calibracao                  › │  selecionado
│   Historico                   › │
│   Configuracoes              › │
│   Registrar amostra          › │
├─────────────────┬───────────────┤
│   < Voltar      │   Entrar >    │
└─────────────────┴───────────────┘
```

**Tela 3 — Calibração (fluxo):**
```
┌─────────────────────────────────┐
│ Calibracao - pH  2/3            │
├─────────────────────────────────┤
│ ████████████████░░░░░░  66%     │  barra de progresso
├─────────────────────────────────┤
│ Passo 2 de 3 - pH 4,00         │
│ Mergulhe o eletrodo e aguarde  │
│ Aguardando...                   │  verde, textSize(2)
├─────────────────────────────────┤
│ Proximo - pH 10,01             │
│ Aguardando confirmacao         │  amarelo
├─────────────────┬───────────────┤
│   X Cancelar    │   Proximo >   │
└─────────────────┴───────────────┘
```

---

## 7. Fluxo de Dados

### 7.1 Boot Sequence

```
setup()
  ├── Serial.begin(115200)
  ├── inicializarDisplay()     → TFT 320x240, rotation(3)
  ├── desenharSplash()         → "Monitor de Qualidade da Água"
  ├── inicializarBotoes()      → INPUT_PULLUP nos 3 GPIOs
  ├── inicializarSensores()    → Wire, ADS1115 (0x48), DS18B20
  ├── carregarCalibracao()     → NVS → SD backup → defaults
  ├── inicializarSD()          → SD.begin(15), criar CSV se necessário
  ├── inicializarBuzzer()      → GPIO 26 OUTPUT LOW
  └── desenharTela(MONITORAMENTO)
```

### 7.2 Main Loop (2 segundos)

```
loop()
  ├── verificarBotoes()        → contínuo
  ├── Se 2s:
  │   ├── lerTodos()
  │   │   ├── sensors.requestTemperatures()
  │   │   ├── dados.temp_c = DS18B20
  │   │   ├── v_ph = ADS1115 AIN2
  │   │   ├── v_tds = ADS1115 AIN1
  │   │   ├── v_turb = ADS1115 AIN0
  │   │   ├── dados.ph = calcPH(v_ph)
  │   │   ├── dados.tds_ppm = calcTDS(v_tds, temp)
  │   │   ├── dados.turbidez_ntu = calcNTU(v_turb)
  │   │   └── verificarAlertas()
  │   └── Se MONITORAMENTO → desenharTela()
  └── servirWeb()              → se FASE2_WEB=1
```

---

## 8. Resultados

### 8.1 Build

| Métrica | Valor |
|---|---|
| RAM | 7,0% (22.824 / 327.680 bytes) |
| Flash | 30,0% (393.177 / 1.310.720 bytes) |
| Erros | 0 |
| Warnings | 0 |

### 8.2 Métricas de Código

| Módulo | Linhas | Responsabilidade |
|---|---|---|
| config.h | 86 | Definições globais |
| sensores.cpp | 69 | Leitura + cálculo |
| display.cpp | 354 | 6 telas + helpers |
| maquina.cpp | 162 | Máquina de estados |
| botoes.cpp | 35 | Debounce + despacho |
| nvs_cal.cpp | 75 | Persistência calibração |
| sd_log.cpp | 87 | Log CSV |
| buzzer.cpp | 31 | Alertas sonoros |
| web.cpp | 38 | Servidor HTTP |
| main.cpp | 43 | Orquestrador |
| **Total** | **~980** | — |

### 8.3 Bibliotecas Utilizadas

| Biblioteca | Versão | Módulo |
|---|---|---|
| Adafruit ST7789 | ≥ 1.11.0 | display |
| Adafruit GFX Library | ≥ 1.11.0 | display |
| Adafruit ADS1X15 | ≥ 2.5.0 | sensores |
| OneWire | ≥ 2.3.7 | sensores |
| DallasTemperature | ≥ 3.9.0 | sensores |
| ArduinoJson | ≥ 6.21.0 | nvs_cal |
| SD | built-in | sd_log |
| Preferences | built-in | nvs_cal |

---

## 9. Trabalhos Futuros

| Item | Descrição | Prioridade |
|---|---|---|
| Fase 2 — Web | Ativar `FASE2_WEB 1`, servidor HTTP WiFi | Alta |
| Histórico real | Conectar Tela 5 ao CSV real (atualmente hardcoded) | Alta |
| Calibração via web | Interface HTML para calibrar remotamente | Média |
| OTA updates | Atualização via WiFi sem cabo | Média |
| Bateria | Monitoramento de nível de bateria | Baixa |
| Mais sensores | Expansão via AIN3 do ADS1115 | Baixa |
