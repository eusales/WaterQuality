#ifndef SENSORES_H
#define SENSORES_H

#include "config.h"

void inicializarSensores();
void lerTodos();
void verificarAlertas();

float calcPH(float v);
float calcTDS(float v, float temp);
float calcNTU(float v);

#endif
