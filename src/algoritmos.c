#include "algoritmos.h"
#include <stdio.h>
#include <stdlib.h>

static void imprimeEstadoMemoria(const Simulador *sim) {
    printf("Tempo t=%d\n", sim->tempo_atual);
    printf("Estado da Memoria Fisica:\n");
    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n");

    for (int i = 0; i < sim->memoria.num_frames; i++) {
        int pid = sim->memoria.frames[i];
        if (pid == -1) {
            printf("|  ----  ");
        } else {
            int page_num = -1;
            Processo *proc = NULL;
            for (int j = 0; j < sim->num_processos; j++) {
                if (sim->processos[j].pid == pid) {
                    proc = &sim->processos[j];
                    break;
                }
            }
            if (proc) {
                for (int p = 0; p < proc->num_paginas; p++) {
                    Pagina *pg = &proc->tabela_paginas[p];
                    if (pg->presente && pg->frame == i) {
                        page_num = p;
                        break;
                    }
                }
            }
            if (page_num >= 0)
                printf("| P%d-%d   ", pid, page_num);
            else
                printf("| ????   ");
        }
    }
    printf("|\n");

    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n\n");
}

void reinicializarMemoria(Simulador *sim) {
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        sim->memoria.frames[i] = -1;
    }

    for (int i = 0; i < sim->num_processos; i++) {
        for (int j = 0; j < sim->processos[i].num_paginas; j++) {
            sim->processos[i].tabela_paginas[j].presente = 0;
            sim->processos[i].tabela_paginas[j].frame = -1;
            sim->processos[i].tabela_paginas[j].ultimo_acesso = 0;
            sim->processos[i].tabela_paginas[j].tempo_carga = 0;
        }
    }

    sim->total_acessos = 0;
    sim->page_faults = 0;
    sim->tempo_atual = 1;
}

int traduzEndereco(Simulador *sim, int pid, int endereco_virtual) {
    switch (sim->algoritmo) {
        case 0: return traduzEnderecoFIFO(sim, pid, endereco_virtual);
        case 1: return traduzEnderecoLRU(sim, pid, endereco_virtual);
        default:
            printf("Algoritmo invalido!\n");
            return -1;
    }
}

int traduzEnderecoFIFO(Simulador *sim, int pid, int endereco_virtual) {
    int tam_pag = sim->tamanho_pagina;
    int num_pag = endereco_virtual / tam_pag;
    int desloc = endereco_virtual % tam_pag;

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

    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d\n",
               sim->tempo_atual, num_pag, pid);
        sim->page_faults++;

        int frame_substituicao = -1;
        int min_tempo = sim->tempo_atual + 1;

        for (int i = 0; i < sim->memoria.num_frames; i++) {
            if (sim->memoria.frames[i] == -1) {
                frame_substituicao = i;
                break;
            } else {
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

        if (frame_substituicao == -1) {
            frame_substituicao = 0;
        }

        for (int p = 0; p < sim->num_processos; p++) {
            Processo *proc_vitima = &sim->processos[p];
            for (int pg = 0; pg < proc_vitima->num_paginas; pg++) {
                Pagina *pag_vitima = &proc_vitima->tabela_paginas[pg];
                if (pag_vitima->presente && pag_vitima->frame == frame_substituicao) {
                    pag_vitima->presente = 0;
                }
            }
        }

        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n",
               sim->tempo_atual, num_pag, pid, frame_substituicao);

        pag->presente = 1;
        pag->frame = frame_substituicao;
        pag->tempo_carga = sim->tempo_atual;
        sim->memoria.frames[frame_substituicao] = pid;
        sim->tempo_atual++;

        imprimeEstadoMemoria(sim);
    } else {
        printf("Tempo t=%d: [HIT] Pagina %d do Processo %d no Frame %d\n",
               sim->tempo_atual, num_pag, pid, pag->frame);
        sim->tempo_atual++;
    }

    int endereco_fisico = (pag->frame * tam_pag) + desloc;
    printf("Endereco Virtual: %d -> Endereco Fisico: %d\n", endereco_virtual, endereco_fisico);
    printf("Detalhes: Pagina=%d, Desloc=%d, Frame=%d\n\n", num_pag, desloc, pag->frame);
    printf("................................................................\n\n");

    return endereco_fisico;
}

int traduzEnderecoLRU(Simulador *sim, int pid, int endereco_virtual) {
    int tam_pag = sim->tamanho_pagina;
    int num_pag = endereco_virtual / tam_pag;
    int desloc = endereco_virtual % tam_pag;

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
    int tempo_atual = sim->tempo_atual++;

    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d\n",
               tempo_atual, num_pag, pid);
        sim->page_faults++;

        int frame_substituicao = -1;
        int min_acesso = tempo_atual + 1;

        for (int i = 0; i < sim->memoria.num_frames; i++) {
            if (sim->memoria.frames[i] == -1) {
                frame_substituicao = i;
                break;
            } else {
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

        if (frame_substituicao == -1) {
            frame_substituicao = 0;
        }

        for (int p = 0; p < sim->num_processos; p++) {
            Processo *proc_vitima = &sim->processos[p];
            for (int pg = 0; pg < proc_vitima->num_paginas; pg++) {
                Pagina *pag_vitima = &proc_vitima->tabela_paginas[pg];
                if (pag_vitima->presente && pag_vitima->frame == frame_substituicao) {
                    pag_vitima->presente = 0;
                }
            }
        }

        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n",
               tempo_atual, num_pag, pid, frame_substituicao);

        pag->presente = 1;
        pag->frame = frame_substituicao;
        pag->ultimo_acesso = tempo_atual;
        sim->memoria.frames[frame_substituicao] = pid;

        imprimeEstadoMemoria(sim);
    } else {
        printf("Tempo t=%d: [HIT] Pagina %d do Processo %d no Frame %d\n",
               tempo_atual, num_pag, pid, pag->frame);
        pag->ultimo_acesso = tempo_atual;
    }

    int endereco_fisico = (pag->frame * tam_pag) + desloc;
    printf("Endereco Virtual: %d -> Endereco Fisico: %d\n", endereco_virtual, endereco_fisico);
    printf("Detalhes: Pagina=%d, Desloc=%d, Frame=%d\n\n", num_pag, desloc, pag->frame);
    

    return endereco_fisico;
}