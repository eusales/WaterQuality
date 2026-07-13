#ifndef SD_LOG_H
#define SD_LOG_H

#include "config.h"

struct Registro {
  char hora[20];
  char rotulo[32];
  float ph;
  float tds_ppm;
  float turbidez_ntu;
  float temp_c;
};

void inicializarSD();
void gravarLinha(const char* rotulo);
void lerHistorico(int pagina, Registro* buf, int* total);

#endif
