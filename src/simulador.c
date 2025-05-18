#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "estruturas.h"
#include "algoritmos.h"

// Inicializa todos os parâmetros do simulador com valores padrões
void inicializarSimulador(Simulador *sim) {
    sim->tempo_atual = 0;                             // Contador global de tempo
    sim->tamanho_pagina = 4096;                       // Tamanho fixo de cada página (4 KB)
    sim->tamanho_memoria_fisica = 16384;              // Total de memória física (16 KB)
    sim->num_processos = 0;                           // Sem processos inicialmente
    sim->processos = NULL;

    // Calcula número de frames e aloca estruturas
    sim->memoria.num_frames = sim->tamanho_memoria_fisica / sim->tamanho_pagina;
    sim->memoria.frames = calloc(sim->memoria.num_frames, sizeof(int));
    sim->memoria.tempo_carga = calloc(sim->memoria.num_frames, sizeof(int));
    for (int i = 0; i < sim->memoria.num_frames; i++)
        sim->memoria.frames[i] = -1;                  // Marca todos os frames como livres

    sim->total_acessos = 0;                           // Contador de acessos a memória
    sim->page_faults   = 0;                           // Contador de page faults
    sim->algoritmo     = 0;                           // Algoritmo padrão = FIFO
}

// Libera toda a memória alocada dinâmicamente pelo simulador
void liberarSimulador(Simulador *sim) {
    for (int i = 0; i < sim->num_processos; i++)
        free(sim->processos[i].tabela_paginas);       // Remove tabelas de páginas
    free(sim->processos);                             // Remove vetor de processos
    free(sim->memoria.frames);                        // Remove vetor de frames
    free(sim->memoria.tempo_carga);                   // Remove vetor de tempo de carga
}

// Desenha o cabeçalho na tela mostrando configurações atuais
void drawHeader(const Simulador *sim) {
    system("cls || clear");                         // Limpa a tela (Windows/Linux)
    const char *algoritmos[] = {"FIFO", "LRU"};

    printf("===== SIMULADOR DE PAGINACAO =====\n");
    printf("Tamanho da pagina: %d bytes (%d KB)\n",
           sim->tamanho_pagina, sim->tamanho_pagina / 1024);
    printf("Tamanho da memoria fisica: %d bytes (%d KB)\n",
           sim->tamanho_memoria_fisica, sim->tamanho_memoria_fisica / 1024);
    printf("Numero de frames: %d\n", sim->memoria.num_frames);
    printf("Algoritmo de substituicao: %s\n\n",
           algoritmos[sim->algoritmo]);
}

// Menu para o usuário alterar parâmetros do simulador
void menuInsercao(Simulador *sim) {
    int opc;
    do {
        drawHeader(sim);
        printf("===== MENU DE PARAMETROS =====\n");
        printf("1. Definir tamanho da pagina\n");
        printf("2. Definir tamanho da memoria fisica\n");
        printf("3. Escolher algoritmo (0=FIFO, 1=LRU, ...)\n");
        printf("0. Voltar ao menu principal\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opc);
        switch (opc) {
            case 1:
                printf("Novo tamanho da pagina (bytes): ");
                scanf("%d", &sim->tamanho_pagina);
                break;
            case 2:
                printf("Novo tamanho da memoria fisica (bytes): ");
                scanf("%d", &sim->tamanho_memoria_fisica);
                break;
            case 3:
                printf("Escolha o algoritmo: ");
                scanf("%d", &sim->algoritmo);
                break;
        }
        // Recalcula frames e realoca vetores após alterações
        sim->memoria.num_frames = sim->tamanho_memoria_fisica / sim->tamanho_pagina;
        free(sim->memoria.frames);
        free(sim->memoria.tempo_carga);
        sim->memoria.frames = calloc(sim->memoria.num_frames, sizeof(int));
        sim->memoria.tempo_carga = calloc(sim->memoria.num_frames, sizeof(int));
        for (int i = 0; i < sim->memoria.num_frames; i++)
            sim->memoria.frames[i] = -1;
    } while (opc != 0);
}

int main(void) {
    setlocale(LC_ALL, "");                            // Configura internacionalização

    Simulador sim;
    inicializarSimulador(&sim);

    // Configuração de exemplo: 1 processo com 8 páginas
    sim.num_processos = 1;
    sim.processos = malloc(sizeof(Processo));
    sim.processos[0].pid = 1;
    sim.processos[0].num_paginas = 8;
    sim.processos[0].tabela_paginas = calloc(8, sizeof(Pagina));

    int escolha;
    do {
        drawHeader(&sim);
        printf("===== MENU PRINCIPAL =====\n");
        printf("1. Inserir parametros\n");
        printf("2. Executar simulacao\n");
        printf("0. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &escolha);

        switch (escolha) {
            case 1:
                menuInsercao(&sim);
                break;

            case 2: {
                drawHeader(&sim);
                printf("======== INICIO DA SIMULACAO ========\n\n");

                // Vetor de acessos de teste (teste misto de faults e hits)
                int acessos[] = {150, 4150, 8150, 12150, 4150,
                                 16150, 150, 8150, 20150};
                int n = sizeof(acessos) / sizeof(*acessos);

                // Executa tradução para cada endereço virtual
                for (int i = 0; i < n; i++) {
                    int endereco_virtual = acessos[i];
                    int endereco_fisico = traduzEndereco(&sim, 1, endereco_virtual);
                    printf("Acesso %2d: Endereco virtual = %5d -> Endereco fisico = %5d\n",
                           i + 1, endereco_virtual, endereco_fisico);
                }

                // Exibe estatísticas ao final da simulação
                printf("\n======== ESTATISTICAS DA SIMULACAO ========\n");
                printf("Total de acessos a memoria: %d\n", sim.total_acessos);
                printf("Total de page faults: %d\n", sim.page_faults);
                double taxa = 100.0 * sim.page_faults / sim.total_acessos;
                printf("Taxa de page faults: %.2f%%\n\n", taxa);
                printf("Algoritmo: FIFO\n");

                printf("\nPressione ENTER para continuar...");
                getchar(); getchar();
            } break;
        }
    } while (escolha != 0);

    liberarSimulador(&sim);                            // Libera todos os recursos
    return 0;
}
