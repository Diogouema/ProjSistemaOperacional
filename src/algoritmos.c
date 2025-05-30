#include "algoritmos.h"
#include <stdio.h>
#include <stdlib.h>

// Imprime estado atual da memória física
static void imprimeEstadoMemoria(const Simulador *sim) {
    printf("Tempo t=%d\n", sim->tempo_atual);
    printf("Estado da Memoria Fisica:\n");
    // Desenha borda superior
    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n");

    // Conteúdo dos frames
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        int pid = sim->memoria.frames[i];
        if (pid == -1) {
            printf("|  ----  "); // Frame livre
        } else {
            int page_num = -1;
            Processo *proc = NULL;
            // Busca processo dono do frame
            for (int j = 0; j < sim->num_processos; j++) {
                if (sim->processos[j].pid == pid) {
                    proc = &sim->processos[j];
                    break;
                }
            }
            // Busca página no frame
            if (proc) {
                for (int p = 0; p < proc->num_paginas; p++) {
                    Pagina *pg = &proc->tabela_paginas[p];
                    if (pg->presente && pg->frame == i) {
                        page_num = p;
                        break;
                    }
                }
            }
            // Formata saída (ex: P1-3 = Processo 1, página 3)
            if (page_num >= 0)
                printf("| P%d-%d   ", pid, page_num);
            else
                printf("| ????   "); // Erro (não deveria ocorrer)
        }
    }
    printf("|\n");

    // Borda inferior
    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n\n");
}

// Reinicializa memória para estado inicial
void reinicializarMemoria(Simulador *sim) {
    // Libera todos os frames
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        sim->memoria.frames[i] = -1;
    }

    // Reseta tabelas de páginas de todos os processos
    for (int i = 0; i < sim->num_processos; i++) {
        for (int j = 0; j < sim->processos[i].num_paginas; j++) {
            sim->processos[i].tabela_paginas[j].presente = 0;
            sim->processos[i].tabela_paginas[j].frame = -1;
            sim->processos[i].tabela_paginas[j].ultimo_acesso = 0;
            sim->processos[i].tabela_paginas[j].tempo_carga = 0;
        }
    }

    // Reseta estatísticas
    sim->total_acessos = 0;
    sim->page_faults = 0;
    sim->tempo_atual = 1;
}

// Seleciona algoritmo para tradução de endereço
int traduzEndereco(Simulador *sim, int pid, int endereco_virtual) {
    switch (sim->algoritmo) {
        case 0: return traduzEnderecoFIFO(sim, pid, endereco_virtual);
        case 1: return traduzEnderecoLRU(sim, pid, endereco_virtual);
        default:
            printf("Algoritmo invalido!\n");
            return -1;
    }
}

// Implementação do algoritmo FIFO
int traduzEnderecoFIFO(Simulador *sim, int pid, int endereco_virtual) {
    int tam_pag = sim->tamanho_pagina;
    int num_pag = endereco_virtual / tam_pag; // Página virtual
    int desloc = endereco_virtual % tam_pag;   // Offset

    // Localiza processo
    Processo *proc = NULL;
    for (int i = 0; i < sim->num_processos; i++) {
        if (sim->processos[i].pid == pid) {
            proc = &sim->processos[i];
            break;
        }
    }
    if (!proc) {
        fprintf(stderr, "Processo %d nao encontrado!\n", pid);
        return -1;
    }

    Pagina *pag = &proc->tabela_paginas[num_pag];
    sim->total_acessos++;

    // Trata page fault
    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d\n",
               sim->tempo_atual, num_pag, pid);
        sim->page_faults++;

        int frame_substituicao = -1;
        int min_tempo = sim->tempo_atual + 1; // Inicializa com valor alto

        // Procura frame livre ou página mais antiga
        for (int i = 0; i < sim->memoria.num_frames; i++) {
            if (sim->memoria.frames[i] == -1) {
                frame_substituicao = i;
                break;
            } else {
                // Procura página com menor tempo de carga (mais antiga)
                for (int p = 0; p < sim->num_processos; p++) {
                    Processo *proc_frame = &sim->processos[p];
                    for (int pg = 0; pg < proc_frame->num_paginas; pg++) {
                        Pagina *pag_frame = &proc_frame->tabela_paginas[pg];
                        if (pag_frame->presente && pag_frame->frame == i) {
                            if (pag_frame->tempo_carga < min_tempo) {
                                min_tempo = pag_frame->tempo_carga;
                                frame_substituicao = i;
                            }
                        }
                    }
                }
            }
        }

        // Fallback se não encontrou
        if (frame_substituicao == -1) {
            frame_substituicao = 0;
        }

        // Remove página atual do frame selecionado
        for (int p = 0; p < sim->num_processos; p++) {
            Processo *proc_vitima = &sim->processos[p];
            for (int pg = 0; pg < proc_vitima->num_paginas; pg++) {
                Pagina *pag_vitima = &proc_vitima->tabela_paginas[pg];
                if (pag_vitima->presente && pag_vitima->frame == frame_substituicao) {
                    pag_vitima->presente = 0;
                }
            }
        }

        // Carrega nova página
        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n",
               sim->tempo_atual, num_pag, pid, frame_substituicao);

        pag->presente = 1;
        pag->frame = frame_substituicao;
        pag->tempo_carga = sim->tempo_atual; // Marca tempo de carga
        sim->memoria.frames[frame_substituicao] = pid;
        sim->tempo_atual++;

        // Mostra estado da memória após substituição
        imprimeEstadoMemoria(sim);
    } else {
        // Hit de página
        printf("Tempo t=%d: [HIT] Pagina %d do Processo %d no Frame %d\n",
               sim->tempo_atual, num_pag, pid, pag->frame);
        sim->tempo_atual++;
    }

    // Calcula endereço físico
    int endereco_fisico = (pag->frame * tam_pag) + desloc;
    printf("Endereco Virtual: %d -> Endereco Fisico: %d\n", endereco_virtual, endereco_fisico);
    printf("Detalhes: Pagina=%d, Desloc=%d, Frame=%d\n\n", num_pag, desloc, pag->frame);
    printf("................................................................\n\n");

    return endereco_fisico;
}

// Implementação do algoritmo LRU
int traduzEnderecoLRU(Simulador *sim, int pid, int endereco_virtual) {
    int tam_pag = sim->tamanho_pagina;
    int num_pag = endereco_virtual / tam_pag;
    int desloc = endereco_virtual % tam_pag;

    // Localiza processo
    Processo *proc = NULL;
    for (int i = 0; i < sim->num_processos; i++) {
        if (sim->processos[i].pid == pid) {
            proc = &sim->processos[i];
            break;
        }
    }
    if (!proc) {
        fprintf(stderr, "Processo %d nao encontrado!\n", pid);
        return -1;
    }

    Pagina *pag = &proc->tabela_paginas[num_pag];
    sim->total_acessos++;
    int tempo_atual = sim->tempo_atual++; // Usa e incrementa tempo

    // Trata page fault
    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d\n",
               tempo_atual, num_pag, pid);
        sim->page_faults++;

        int frame_substituicao = -1;
        int min_acesso = tempo_atual + 1; // Inicializa com valor alto

        // Procura frame livre ou página menos recentemente usada
        for (int i = 0; i < sim->memoria.num_frames; i++) {
            if (sim->memoria.frames[i] == -1) {
                frame_substituicao = i;
                break;
            } else {
                // Procura página com acesso mais antigo
                for (int p = 0; p < sim->num_processos; p++) {
                    Processo *proc_frame = &sim->processos[p];
                    for (int pg = 0; pg < proc_frame->num_paginas; pg++) {
                        Pagina *pag_frame = &proc_frame->tabela_paginas[pg];
                        if (pag_frame->presente && pag_frame->frame == i) {
                            if (pag_frame->ultimo_acesso < min_acesso) {
                                min_acesso = pag_frame->ultimo_acesso;
                                frame_substituicao = i;
                            }
                        }
                    }
                }
            }
        }

        // Fallback se não encontrou
        if (frame_substituicao == -1) {
            frame_substituicao = 0;
        }

        // Remove página atual do frame
        for (int p = 0; p < sim->num_processos; p++) {
            Processo *proc_vitima = &sim->processos[p];
            for (int pg = 0; pg < proc_vitima->num_paginas; pg++) {
                Pagina *pag_vitima = &proc_vitima->tabela_paginas[pg];
                if (pag_vitima->presente && pag_vitima->frame == frame_substituicao) {
                    pag_vitima->presente = 0;
                }
            }
        }

        // Carrega nova página
        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n",
               tempo_atual, num_pag, pid, frame_substituicao);

        pag->presente = 1;
        pag->frame = frame_substituicao;
        pag->ultimo_acesso = tempo_atual; // Atualiza último acesso
        sim->memoria.frames[frame_substituicao] = pid;

        // Mostra estado da memória
        imprimeEstadoMemoria(sim);
    } else {
        // Hit de página - atualiza tempo de acesso
        printf("Tempo t=%d: [HIT] Pagina %d do Processo %d no Frame %d\n",
               tempo_atual, num_pag, pid, pag->frame);
        pag->ultimo_acesso = tempo_atual;
    }

    // Calcula endereço físico
    int endereco_fisico = (pag->frame * tam_pag) + desloc;
    printf("Endereco Virtual: %d -> Endereco Fisico: %d\n", endereco_virtual, endereco_fisico);
    printf("Detalhes: Pagina=%d, Desloc=%d, Frame=%d\n\n", num_pag, desloc, pag->frame);
    

    return endereco_fisico;
}