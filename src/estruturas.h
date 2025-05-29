#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

typedef struct
{
    int presente;      // 1 se a página está na memória , 0 caso contrário
    int frame;         // Número do frame onde a página está alocada (-1 se não alocada)
    int modificada;    // 1 se a página foi modificada , 0 caso contrário
    int referenciada;  // 1 se a página foi referenciada recentemente , 0 caso contrário
    int tempo_carga;   // Instante em que a página foi carregada na memória
    int ultimo_acesso; // Instante do último acesso à página
} Pagina;

typedef struct
{
    int pid;                // Identificador do processo
    int tamanho;            // Tamanho do processo em bytes
    int num_paginas;        // Número de páginas do processo
    Pagina *tabela_paginas; // Tabela de páginas do processo
} Processo;

typedef struct
{
    int num_frames; // Número total de frames na memória física
    int *frames;    // Array de frames (cada elemento contém o pid e a página)
                    // Ex: frames[i] = (pid << 16) | num_pagina
    int *tempo_carga; // Tempo em que cada frame foi carregado (para FIFO)
} MemoriaFisica;

typedef struct
{
    int tempo_atual;            // Contador de tempo da simulação
    int tamanho_pagina;         // Tamanho da página em bytes
    int tamanho_memoria_fisica; // Tamanho da memória física em bytes
    int num_processos;          // Número de processos na simulação
    Processo *processos;        // Array de processos
    MemoriaFisica memoria;      // Memória física
    // Estatísticas
    int total_acessos; // Total de acessos à memória
    int page_faults;   // Total de page faults ocorridos
    // Algoritmo de substituição atual
    int algoritmo; // 0=FIFO , 1=LRU
} Simulador;

#endif