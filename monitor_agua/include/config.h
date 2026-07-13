#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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

#endif
