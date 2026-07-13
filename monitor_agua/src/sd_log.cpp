#include "sd_log.h"
#include <SD.h>
#include <SPI.h>

void inicializarSD() {
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] Falha na inicializacao!");
    return;
  }
  Serial.println("[SD] Inicializado com sucesso");

  uint8_t cardType = SD.cardType();
  const char* tipo = "Desconhecido";
  if (cardType == CARD_MMC)  tipo = "MMC";
  if (cardType == CARD_SD)   tipo = "SD";
  if (cardType == CARD_SDHC) tipo = "SDHC";
  Serial.printf("[SD] Tipo: %s, Total: %llu MB\n", tipo, SD.totalBytes() / (1024 * 1024));

  if (!SD.exists(CSV_FILENAME)) {
    File f = SD.open(CSV_FILENAME, FILE_WRITE);
    if (f) {
      f.println("timestamp,rotulo,ph,tds_ppm,turbidez_ntu,temperatura_c");
      f.close();
      Serial.println("[SD] Arquivo CSV criado com cabecalho");
    }
  }
}

void gravarLinha(const char* rotulo) {
  File f = SD.open(CSV_FILENAME, FILE_APPEND);
  if (!f) {
    Serial.println("[SD] Erro ao abrir arquivo para escrita");
    return;
  }

  char linha[128];
  snprintf(linha, sizeof(linha), "%s,%.2f,%.1f,%.1f,%.1f",
           rotulo, dados.ph, dados.tds_ppm, dados.turbidez_ntu, dados.temp_c);
  f.println(linha);
  f.close();
  Serial.printf("[SD] Gravado: %s\n", linha);
}

void lerHistorico(int pagina, Registro* buf, int* total) {
  File f = SD.open(CSV_FILENAME, FILE_READ);
  if (!f) { *total = 0; return; }

  int count = 0;
  int perPage = 5;
  int startLine = pagina * perPage + 1;
  int lineNum = 0;

  while (f.available() && count < perPage) {
    String linha = f.readStringUntil('\n');
    linha.trim();
    if (linha.length() == 0) continue;

    if (lineNum >= startLine) {
      int idx = 0;
      int campo = 0;
      int len = linha.length();

      for (int i = 0; i < len && campo < 6; i++) {
        if (linha[i] == ',' || i == len - 1) {
          int end = (i == len - 1) ? i + 1 : i;
          String val = linha.substring(idx, end);
          val.trim();
          switch (campo) {
            case 0: strncpy(buf[count].hora, val.c_str(), sizeof(buf[count].hora) - 1); break;
            case 1: strncpy(buf[count].rotulo, val.c_str(), sizeof(buf[count].rotulo) - 1); break;
            case 2: buf[count].ph = val.toFloat(); break;
            case 3: buf[count].tds_ppm = val.toFloat(); break;
            case 4: buf[count].turbidez_ntu = val.toFloat(); break;
            case 5: buf[count].temp_c = val.toFloat(); break;
          }
          idx = i + 1;
          campo++;
        }
      }
      count++;
    }
    lineNum++;
  }

  *total = count;
  f.close();
}
