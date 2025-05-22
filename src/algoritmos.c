#include "algoritmos.h"
#include <stdio.h>
#include <stdlib.h>

// Imprime o estado atual da memória física em tabela ASCII
static void imprimeEstadoMemoria(const Simulador *sim)
{
    printf("Tempo t=%d\n", sim->tempo_atual - 1);
    printf("Estado da Memoria Fisica:\n");
    // Topo da tabela
    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n");

    // Linhas de conteúdo
    for (int i = 0; i < sim->memoria.num_frames; i++)
    {
        int pid = sim->memoria.frames[i];
        if (pid == -1)
        {
            printf("|  ----  ");
        }
        else
        {
            // Encontra o nº da página nela carregada
            int page_num = -1;
            Processo *proc = NULL;
            for (int j = 0; j < sim->num_processos; j++)
            {
                if (sim->processos[j].pid == pid)
                {
                    proc = &sim->processos[j];
                    break;
                }
            }
            if (proc)
            {
                for (int p = 0; p < proc->num_paginas; p++)
                {
                    Pagina *pg = &proc->tabela_paginas[p];
                    if (pg->presente && pg->frame == i)
                    {
                        page_num = p;
                        break;
                    }
                }
            }
            if (page_num >= 0)
                printf("| P%d-%d   ", pid, page_num);
            else
                printf(" ????   ");
        }
    }
    printf("|\n");

    // Base da tabela
    for (int i = 0; i < sim->memoria.num_frames; i++)
        printf(" ------- ");
    printf("\n\n");
}

// Dispatcher para o algoritmo escolhido
int traduzEndereco(Simulador *sim, int pid, int endereco_virtual)
{
    switch (sim->algoritmo)
    {
    case 0:
        return traduzEnderecoFIFO(sim, pid, endereco_virtual);
    case 1:
        return traduzEnderecoLRU(sim, pid, endereco_virtual);
    default:
        printf("Algoritmo invalido!\n");
        return -1;
    }
}

// Implementação FIFO com logs e dump de memória
int traduzEnderecoFIFO(Simulador *sim, int pid, int endereco_virtual)
{
    int tam_pag = sim->tamanho_pagina;
    int num_pag = endereco_virtual / tam_pag;
    int desloc = endereco_virtual % tam_pag;

    // Encontra processo
    Processo *proc = NULL;
    for (int i = 0; i < sim->num_processos; i++)
    {
        if (sim->processos[i].pid == pid)
        {
            proc = &sim->processos[i];
            break;
        }
    }
    if (!proc)
    {
        fprintf(stderr, "Processo %d nao encontrado!\n", pid);
        return -1;
    }

    Pagina *pag = &proc->tabela_paginas[num_pag];
    sim->total_acessos++;

    if (!pag->presente)
    {
        // Log de page fault
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d nao esta na memoria fisica!\n",
               sim->tempo_atual, num_pag, pid);

        sim->page_faults++;
        int frame_livre = -1;
        for (int i = 0; i < sim->memoria.num_frames; i++)
        {
            if (sim->memoria.frames[i] == -1)
            {
                frame_livre = i;
                break;
            }
        }
        if (frame_livre == -1)
        {
            // Escolhe o frame mais antigo (FIFO)
            int mais_antigo = 0;
            for (int i = 1; i < sim->memoria.num_frames; i++)
            {
                if (sim->memoria.tempo_carga[i] < sim->memoria.tempo_carga[mais_antigo])
                    mais_antigo = i;
            }
            frame_livre = mais_antigo;
            // Invalida a página que estava lá
            for (int j = 0; j < proc->num_paginas; j++)
            {
                Pagina *pg = &proc->tabela_paginas[j];
                if (pg->presente && pg->frame == frame_livre)
                {
                    pg->presente = 0;
                    break;
                }
            }
        }
        // Log de carregamento
        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n\n",
               sim->tempo_atual, num_pag, pid, frame_livre);

        // Atualiza estruturas
        pag->presente = 1;
        pag->frame = frame_livre;
        sim->memoria.frames[frame_livre] = pid;
        sim->memoria.tempo_carga[frame_livre] = sim->tempo_atual++;

        // Dump de memória
        imprimeEstadoMemoria(sim);
    }

    int endereco_fisico = pag->frame * tam_pag + desloc;
    printf("Tempo t=%d: Endereco Virtual (P%d): %d -> Pagina: %d -> Frame: %d -> Endereco Fisico: %d\n\n",
           sim->tempo_atual - 1, pid, endereco_virtual, num_pag, pag->frame, endereco_fisico);
    printf("........................................................................................\n\n");

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

    // Incrementa o tempo global ANTES de usar
    sim->tempo_atual++;
    int tempo_atual_acesso = sim->tempo_atual;

    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Pagina %d do Processo %d nao esta na memoria fisica!\n",
               sim->tempo_atual, num_pag, pid);

        sim->page_faults++;
        int frame_livre = -1;

        // Procura frame livre
        for (int i = 0; i < sim->memoria.num_frames; i++) {
            if (sim->memoria.frames[i] == -1) {
                frame_livre = i;
                break;
            }
        }

        // Substituição LRU se necessário
        if (frame_livre == -1) {
            int lru_frame = 0;
            int menor_tempo = sim->tempo_atual + 1;

            for (int i = 0; i < sim->memoria.num_frames; i++) {
                int pid_frame = sim->memoria.frames[i];
                if (pid_frame == -1) continue;

                Pagina *pag_frame = NULL;
                for (int p = 0; p < sim->num_processos; p++) {
                    Processo *proc_frame = &sim->processos[p];
                    if (proc_frame->pid != pid_frame) continue;

                    for (int pgn = 0; pgn < proc_frame->num_paginas; pgn++) {
                        Pagina *pg = &proc_frame->tabela_paginas[pgn];
                        if (pg->presente && pg->frame == i) {
                            pag_frame = pg;
                            break;
                        }
                    }
                    if (pag_frame) break;
                }

                if (pag_frame && pag_frame->ultimo_acesso < menor_tempo) {
                    menor_tempo = pag_frame->ultimo_acesso;
                    lru_frame = i;
                }
            }
            frame_livre = lru_frame;

            // Invalida TODAS as páginas no frame substituído
            for (int p = 0; p < sim->num_processos; p++) {
                Processo *proc_vitima = &sim->processos[p];
                for (int pg = 0; pg < proc_vitima->num_paginas; pg++) {
                    Pagina *pag_vitima = &proc_vitima->tabela_paginas[pg];
                    if (pag_vitima->presente && pag_vitima->frame == frame_livre) {
                        pag_vitima->presente = 0;
                    }
                }
            }
        }

        printf("Tempo t=%d: Carregando Pagina %d do Processo %d no Frame %d\n\n",
               sim->tempo_atual, num_pag, pid, frame_livre);

        // Atualiza estruturas
        pag->presente = 1;
        pag->frame = frame_livre;
        pag->ultimo_acesso = tempo_atual_acesso;
        sim->memoria.frames[frame_livre] = pid;

        imprimeEstadoMemoria(sim);
    } else {
        // Atualiza último acesso mesmo em HIT
        pag->ultimo_acesso = tempo_atual_acesso;
    }

    int endereco_fisico = pag->frame * tam_pag + desloc;
    printf("Tempo t=%d: Endereco Virtual (P%d): %d -> Pagina: %d -> Frame: %d -> Endereco Fisico: %d\n\n",
           sim->tempo_atual, pid, endereco_virtual, num_pag, pag->frame, endereco_fisico);
    printf("........................................................................................\n\n");

    return endereco_fisico;
}