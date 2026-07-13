#include "sensores.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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
  Serial.println("[SENSORES] Inicializados: ADS1115 + DS18B20");
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

  Serial.printf("[LEITURA] pH:%.2f TDS:%.1fppm Turb:%.1fNTU Temp:%.1fC | %s %s %s\n",
                dados.ph, dados.tds_ppm, dados.turbidez_ntu, dados.temp_c,
                dados.ph_ok ? "pH_OK" : "pH_NOK",
                dados.tds_ok ? "TDS_OK" : "TDS_NOK",
                dados.turb_ok ? "TURB_OK" : "TURB_NOK");
}
