#include "nvs_cal.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SD.h>

static Preferences prefs;

void carregarCalibracao() {
  prefs.begin("calibracao", true);

  if (prefs.isKey("ph_offset")) {
    calib.ph_offset    = prefs.getFloat("ph_offset",    0);
    calib.ph_slope_acid = prefs.getFloat("ph_slope_acid", 1);
    calib.ph_slope_bas = prefs.getFloat("ph_slope_bas", 1);
    calib.k_tds        = prefs.getFloat("k_tds",        1);
    calib.turb_a       = prefs.getFloat("turb_a",       0);
    calib.turb_b       = prefs.getFloat("turb_b",       0);
    calib.turb_c       = prefs.getFloat("turb_c",       0);
    Serial.println("[NVS] Calibracao carregada da NVS");
  } else {
    Serial.println("[NVS] NVS vazia, tentando backup no SD...");
    File f = SD.open(CAL_BACKUP_FILE, FILE_READ);
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (!err) {
        calib.ph_offset    = doc["ph_offset"]    | 0.0f;
        calib.ph_slope_acid = doc["ph_slope_acid"] | 1.0f;
        calib.ph_slope_bas = doc["ph_slope_bas"] | 1.0f;
        calib.k_tds        = doc["k_tds"]        | 1.0f;
        calib.turb_a       = doc["turb_a"]       | 0.0f;
        calib.turb_b       = doc["turb_b"]       | 0.0f;
        calib.turb_c       = doc["turb_c"]       | 0.0f;
        Serial.println("[NVS] Calibracao restaurada do backup SD");
      } else {
        Serial.println("[NVS] Erro ao ler backup, usando valores padrao");
      }
    } else {
      Serial.println("[NVS] Sem backup, usando valores padrao");
    }
  }

  prefs.end();
}

void salvarCalibracao() {
  prefs.begin("calibracao", false);

  prefs.putFloat("ph_offset",    calib.ph_offset);
  prefs.putFloat("ph_slope_acid", calib.ph_slope_acid);
  prefs.putFloat("ph_slope_bas", calib.ph_slope_bas);
  prefs.putFloat("k_tds",        calib.k_tds);
  prefs.putFloat("turb_a",       calib.turb_a);
  prefs.putFloat("turb_b",       calib.turb_b);
  prefs.putFloat("turb_c",       calib.turb_c);

  prefs.end();
  Serial.println("[NVS] Calibracao salva na NVS");

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
    Serial.println("[NVS] Backup salvo no SD (cal_backup.json)");
  }
}
