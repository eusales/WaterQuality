#ifndef MAQUINA_H
#define MAQUINA_H

#include "config.h"

void acaoNavegar();
void acaoConfirmar();
void acaoVoltar();
Estado getEstado();

// ── Variáveis de estado compartilhadas (display le, máquina escreve) ──
extern int   menuSel;
extern int   calibSel;
extern int   calibPasso;
extern int   rotuloSel;
extern int   histSel;
extern int   confSel;
extern bool  confEditando;

extern float lim_ph_min;
extern float lim_ph_max;
extern float lim_tds;
extern float lim_turb;
extern int   lim_interv;

#endif
