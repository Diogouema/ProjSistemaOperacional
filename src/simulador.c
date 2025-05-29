#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "estruturas.h"
#include "algoritmos.h"

void inicializarSimulador(Simulador *sim) {
    sim->tempo_atual = 1;
    sim->tamanho_pagina = 4096;
    sim->tamanho_memoria_fisica = 12288;
    sim->num_processos = 0;
    sim->processos = NULL;

    sim->memoria.num_frames = sim->tamanho_memoria_fisica / sim->tamanho_pagina;
    sim->memoria.frames = malloc(sim->memoria.num_frames * sizeof(int));
    
    for (int i = 0; i < sim->memoria.num_frames; i++)
        sim->memoria.frames[i] = -1;

    sim->total_acessos = 0;
    sim->page_faults = 0;
    sim->algoritmo = 0;
}

void liberarSimulador(Simulador *sim) {
    for (int i = 0; i < sim->num_processos; i++) {
        free(sim->processos[i].tabela_paginas);
    }
    free(sim->processos);
    free(sim->memoria.frames);
}

void drawHeader(const Simulador *sim) {
    system("cls || clear");
    const char *algoritmos[] = {"FIFO", "LRU"};
    printf("===== SIMULADOR DE PAGINACAO =====\n");
    printf("Tamanho da pagina: %d bytes\n", sim->tamanho_pagina);
    printf("Memoria fisica: %d bytes (%d frames)\n", 
           sim->tamanho_memoria_fisica, sim->memoria.num_frames);
    printf("Algoritmo: %s\n\n", algoritmos[sim->algoritmo]);
}

void menuAlgoritmo(Simulador *sim) {
    system("cls || clear");
    printf("===== SELECIONE O ALGORITMO =====\n");
    printf("0 - FIFO (First-In, First-Out)\n");
    printf("1 - LRU (Least Recently Used)\n");
    printf("Escolha: ");
    scanf("%d", &sim->algoritmo);
    
    if(sim->algoritmo < 0 || sim->algoritmo > 1) {
        printf("Selecao invalida! Usando FIFO.\n");
        sim->algoritmo = 0;
    }
    
    reinicializarMemoria(sim);
}

void menuParametros(Simulador *sim) {
    int opcao;
    do {
        drawHeader(sim);
        printf("===== PARAMETROS =====\n");
        printf("1. Tamanho da pagina\n");
        printf("2. Tamanho da memoria fisica\n");
        printf("3. Algoritmo de substituicao\n");
        printf("0. Voltar\n");
        printf("Escolha: ");
        scanf("%d", &opcao);
        
        switch(opcao) {
            case 1:
                printf("Novo tamanho (bytes): ");
                scanf("%d", &sim->tamanho_pagina);
                break;
            case 2:
                printf("Novo tamanho (bytes): ");
                scanf("%d", &sim->tamanho_memoria_fisica);
                break;
            case 3:
                menuAlgoritmo(sim);
                break;
        }
        
        int novos_frames = sim->tamanho_memoria_fisica / sim->tamanho_pagina;
        if(novos_frames != sim->memoria.num_frames) {
            sim->memoria.num_frames = novos_frames;
            free(sim->memoria.frames);
            sim->memoria.frames = malloc(sim->memoria.num_frames * sizeof(int));
            reinicializarMemoria(sim);
        }
    } while(opcao != 0);
}

int main() {
    setlocale(LC_ALL, "Portuguese");
    Simulador sim;
    inicializarSimulador(&sim);
    
    // Configurar processo de exemplo
    sim.num_processos = 1;
    sim.processos = malloc(sizeof(Processo));
    sim.processos[0].pid = 1;
    sim.processos[0].num_paginas = 8;
    sim.processos[0].tabela_paginas = calloc(8, sizeof(Pagina));
    
    int escolha;
    do {
        drawHeader(&sim);
        printf("===== MENU PRINCIPAL =====\n");
        printf("1. Parametros\n");
        printf("2. Executar simulacao\n");
        printf("0. Sair\n");
        printf("Escolha: ");
        scanf("%d", &escolha);
        
        switch(escolha) {
            case 1:
                menuParametros(&sim);
                break;
                
            case 2: {
                reinicializarMemoria(&sim);
                drawHeader(&sim);
                printf("===== SIMULACAO =====\n");
                
                // Sequência de teste melhorada
                int acessos[] = {0, 4096, 8192, 0, 12288, 16384, 0, 4096, 8192, 12288, 16384};
                int num_acessos = sizeof(acessos)/sizeof(acessos[0]);
                
                for(int i = 0; i < num_acessos; i++) {
                    printf("\n--- Acesso %d: Virtual=%d ---\n", i+1, acessos[i]);
                    int fisico = traduzEndereco(&sim, 1, acessos[i]);
                    printf("Resultado: Virtual=%d -> Fisico=%d\n", acessos[i], fisico);
                    printf("................................................................\n\n");
                }
                
                printf("\n===== RESULTADOS =====\n");
                printf("Total de acessos: %d\n", sim.total_acessos);
                printf("Page faults: %d\n", sim.page_faults);
                printf("Taxa de page faults: %.2f%%\n", 
                       (sim.page_faults * 100.0) / sim.total_acessos);
                
                printf("\nPressione ENTER para continuar...");
                getchar();
                getchar();
                break;
            }
        }
    } while(escolha != 0);
    
    liberarSimulador(&sim);
    return 0;
}