#ifndef ALGORITMOS_H
#define ALGORITMOS_H

#include "estruturas.h"

int traduzEndereco(Simulador *sim, int pid, int endereco_virtual);
int traduzEnderecoFIFO(Simulador *sim, int pid, int endereco_virtual);
int traduzEnderecoLRU(Simulador *sim, int pid, int endereco_virtual);
void reinicializarMemoria(Simulador *sim);


#endif
