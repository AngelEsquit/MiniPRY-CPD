/*
 * ecosystem.c
 * Implementación modularizada de la simulación de ecosistema
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "ecosystem.h"

/* Constantes (copiadas del original) */
#define EMPTY     0
#define PLANT     1
#define HERBIVORE 2
#define CARNIVORE 3

#define PLANT_SPREAD_PROB   0.30
#define HERB_REPRODUCE_PROB 0.25
#define CARN_REPRODUCE_PROB 0.20
#define HERB_ENERGY_INIT    3
#define CARN_ENERGY_INIT    5
#define HERB_ENERGY_EAT     1
#define CARN_ENERGY_EAT     2
#define HERB_STARVE_TICKS   3
#define CARN_STARVE_TICKS   5
#define NO_MOVE            -1

typedef struct {
    int src_i, src_j;
    int dst_i, dst_j;
    int priority;
} Intent;

/* Variables internas */
static int   ROWS, COLS, NUM_THREADS;
static Cell *grid_cur  = NULL;
static Cell *grid_next = NULL;
static int  *claim     = NULL;
static Intent *intents = NULL;
static unsigned int *seeds = NULL;

static inline int IDX(int i, int j) { return i * COLS + j; }
static inline Cell *CUR(int i, int j)  { return &grid_cur [IDX(i,j)]; }
static inline Cell *NEXT(int i, int j) { return &grid_next[IDX(i,j)]; }
static inline double rand_d(unsigned int *seed) {
    return (double)rand_r(seed) / (double)RAND_MAX;
}

static const int DR[] = {-1, 1,  0, 0};
static const int DC[] = { 0, 0, -1, 1};

static void shuffle(int *arr, int n, unsigned int *seed) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand_r(seed) % (i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

static void init_ecosystem(int n_plants, int n_herb, int n_carn) {
    int total = ROWS * COLS;

    grid_cur  = calloc(total, sizeof(Cell));
    grid_next = calloc(total, sizeof(Cell));
    claim     = malloc(total * sizeof(int));
    intents   = malloc(total * sizeof(Intent));

    if (!grid_cur || !grid_next || !claim || !intents) {
        fprintf(stderr, "Error: sin memoria\n");
        exit(1);
    }

    int *indices = malloc(total * sizeof(int));
    for (int i = 0; i < total; i++) indices[i] = i;

    unsigned int seed = 42;
    shuffle(indices, total, &seed);

    int placed = 0;
    for (int i = 0; i < n_plants && placed < total; i++, placed++) {
        grid_cur[indices[placed]] = (Cell){ PLANT, 0, 0, 0 };
    }
    for (int i = 0; i < n_herb && placed < total; i++, placed++) {
        grid_cur[indices[placed]] = (Cell){ HERBIVORE, HERB_ENERGY_INIT, 0, 0 };
    }
    for (int i = 0; i < n_carn && placed < total; i++, placed++) {
        grid_cur[indices[placed]] = (Cell){ CARNIVORE, CARN_ENERGY_INIT, 0, 0 };
    }

    free(indices);
}

static void phase1_compute_intents(unsigned int *seeds_local) {
    #pragma omp parallel for collapse(2) schedule(dynamic, 16) \
                             num_threads(NUM_THREADS)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int tid  = omp_get_thread_num();
            unsigned int *seed = &seeds_local[tid];

            Intent *it = &intents[IDX(i,j)];
            it->src_i    = i;
            it->src_j    = j;
            it->dst_i    = NO_MOVE;
            it->dst_j    = NO_MOVE;
            it->priority = 0;

            Cell *c = CUR(i, j);
            if (c->type == EMPTY) continue;

            if (c->type == PLANT) {
                if (rand_d(seed) < PLANT_SPREAD_PROB) {
                    int order[4] = {0,1,2,3};
                    shuffle(order, 4, seed);
                    for (int d = 0; d < 4; d++) {
                        int ni = i + DR[order[d]];
                        int nj = j + DC[order[d]];
                        if (ni < 0 || ni >= ROWS || nj < 0 || nj >= COLS) continue;
                        if (CUR(ni,nj)->type == EMPTY) {
                            it->dst_i    = ni;
                            it->dst_j    = nj;
                            it->priority = 1;
                            break;
                        }
                    }
                }
                continue;
            }

            if (c->type == HERBIVORE) {
                int order[4] = {0,1,2,3};
                shuffle(order, 4, seed);

                for (int d = 0; d < 4; d++) {
                    int ni = i + DR[order[d]];
                    int nj = j + DC[order[d]];
                    if (ni < 0 || ni >= ROWS || nj < 0 || nj >= COLS) continue;
                    if (CUR(ni,nj)->type == PLANT) {
                        it->dst_i    = ni;
                        it->dst_j    = nj;
                        it->priority = 10 + c->energy;
                        goto herb_done;
                    }
                }
                for (int d = 0; d < 4; d++) {
                    int ni = i + DR[order[d]];
                    int nj = j + DC[order[d]];
                    if (ni < 0 || ni >= ROWS || nj < 0 || nj >= COLS) continue;
                    if (CUR(ni,nj)->type == EMPTY) {
                        it->dst_i    = ni;
                        it->dst_j    = nj;
                        it->priority = 5;
                        break;
                    }
                }
                herb_done:;
                continue;
            }

            if (c->type == CARNIVORE) {
                int order[4] = {0,1,2,3};
                shuffle(order, 4, seed);

                for (int d = 0; d < 4; d++) {
                    int ni = i + DR[order[d]];
                    int nj = j + DC[order[d]];
                    if (ni < 0 || ni >= ROWS || nj < 0 || nj >= COLS) continue;
                    if (CUR(ni,nj)->type == HERBIVORE) {
                        it->dst_i    = ni;
                        it->dst_j    = nj;
                        it->priority = 20 + c->energy;
                        goto carn_done;
                    }
                }
                for (int d = 0; d < 4; d++) {
                    int ni = i + DR[order[d]];
                    int nj = j + DC[order[d]];
                    if (ni < 0 || ni >= ROWS || nj < 0 || nj >= COLS) continue;
                    if (CUR(ni,nj)->type == EMPTY) {
                        it->dst_i    = ni;
                        it->dst_j    = nj;
                        it->priority = 15;
                        break;
                    }
                }
                carn_done:;
            }
        }
    }
}

static void phase2_resolve_claims(void) {
    int total = ROWS * COLS;
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int k = 0; k < total; k++) claim[k] = -1;

    #pragma omp parallel for schedule(dynamic, 16) num_threads(NUM_THREADS)
    for (int k = 0; k < total; k++) {
        Intent *it = &intents[k];
        if (it->dst_i == NO_MOVE) continue;

        int dst = IDX(it->dst_i, it->dst_j);
        int old_val;
        int my_val = k;
        int success = 0;

        while (!success) {
            #pragma omp atomic read
            old_val = claim[dst];

            if (old_val == -1) {
                #pragma omp critical (claim_update)
                {
                    if (claim[dst] == -1) {
                        claim[dst] = my_val;
                        success = 1;
                    }
                }
            } else {
                int old_priority = intents[old_val].priority;
                if (it->priority > old_priority) {
                    #pragma omp critical (claim_update)
                    {
                        if (claim[dst] == old_val &&
                            intents[old_val].priority < it->priority) {
                            claim[dst] = my_val;
                        }
                    }
                }
                success = 1;
            }
        }
    }
}

static void phase3_apply(unsigned int *seeds_local) {
    int total = ROWS * COLS;
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int k = 0; k < total; k++)
        grid_next[k] = (Cell){ EMPTY, 0, 0, 0 };

    #pragma omp parallel for collapse(2) schedule(dynamic, 16) \
                             num_threads(NUM_THREADS)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int tid  = omp_get_thread_num();
            unsigned int *seed = &seeds_local[tid];

            int dst = IDX(i, j);
            int src = claim[dst];

            Cell *dst_cur = CUR(i, j);

            if (src == -1) {
                if (dst_cur->type == EMPTY) continue;

                Cell new_c = *dst_cur;
                new_c.age++;

                if (new_c.type == PLANT) {
                    NEXT(i,j)[0] = new_c;
                    continue;
                }

                new_c.starve++;
                new_c.energy--;

                int starve_limit = (new_c.type == HERBIVORE)
                                   ? HERB_STARVE_TICKS : CARN_STARVE_TICKS;

                if (new_c.energy <= 0 || new_c.starve >= starve_limit) {
                    NEXT(i,j)[0] = (Cell){ EMPTY, 0, 0, 0 };
                } else {
                    NEXT(i,j)[0] = new_c;
                }
                continue;
            }

            int si = intents[src].src_i;
            int sj = intents[src].src_j;
            Cell *src_cur = CUR(si, sj);
            Cell new_c    = *src_cur;
            new_c.age++;

            if (new_c.type == PLANT) {
                NEXT(i,j)[0] = new_c;
                continue;
            }

            if (new_c.type == HERBIVORE && dst_cur->type == PLANT) {
                new_c.energy += HERB_ENERGY_EAT;
                new_c.starve  = 0;

                if (new_c.energy >= HERB_ENERGY_INIT * 2 &&
                    rand_d(seed) < HERB_REPRODUCE_PROB) {
                    Cell cria = { HERBIVORE, HERB_ENERGY_INIT, 0, 0 };
                    NEXT(si,sj)[0] = cria;
                    new_c.energy  -= HERB_ENERGY_INIT;
                }

                NEXT(i,j)[0] = new_c;
                continue;
            }

            if (new_c.type == HERBIVORE && dst_cur->type == EMPTY) {
                new_c.starve++;
                new_c.energy--;
                if (new_c.energy <= 0 || new_c.starve >= HERB_STARVE_TICKS) {
                    NEXT(i,j)[0] = (Cell){ EMPTY, 0, 0, 0 };
                } else {
                    NEXT(i,j)[0] = new_c;
                }
                continue;
            }

            if (new_c.type == CARNIVORE && dst_cur->type == HERBIVORE) {
                new_c.energy += CARN_ENERGY_EAT;
                new_c.starve  = 0;

                if (new_c.energy >= CARN_ENERGY_INIT * 2 &&
                    rand_d(seed) < CARN_REPRODUCE_PROB) {
                    Cell cria = { CARNIVORE, CARN_ENERGY_INIT, 0, 0 };
                    NEXT(si,sj)[0] = cria;
                    new_c.energy  -= CARN_ENERGY_INIT;
                }

                NEXT(i,j)[0] = new_c;
                continue;
            }

            if (new_c.type == CARNIVORE && dst_cur->type == EMPTY) {
                new_c.starve++;
                new_c.energy--;
                if (new_c.energy <= 0 || new_c.starve >= CARN_STARVE_TICKS) {
                    NEXT(i,j)[0] = (Cell){ EMPTY, 0, 0, 0 };
                } else {
                    NEXT(i,j)[0] = new_c;
                }
                continue;
            }

            if (NEXT(si,sj)->type == EMPTY) {
                new_c.starve++;
                NEXT(si,sj)[0] = new_c;
            }
        }
    }

    #pragma omp parallel for collapse(2) num_threads(NUM_THREADS)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (CUR(i,j)->type == PLANT && NEXT(i,j)->type == EMPTY) {
                NEXT(i,j)[0] = *CUR(i,j);
                NEXT(i,j)->age++;
            }
        }
    }
}

static void print_state(int tick) {
    int plants = 0, herb = 0, carn = 0;
    #pragma omp parallel for reduction(+:plants,herb,carn) \
                             num_threads(NUM_THREADS)
    for (int k = 0; k < ROWS * COLS; k++) {
        if (grid_cur[k].type == PLANT)     plants++;
        if (grid_cur[k].type == HERBIVORE) herb++;
        if (grid_cur[k].type == CARNIVORE) carn++;
    }

    printf("\n** Tick %d **\n", tick);
    printf("Plantas: %d | Herbívoros: %d | Carnívoros: %d\n",
           plants, herb, carn);

    if (ROWS <= 30 && COLS <= 30) {
        printf("Distribución:\n");
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                char ch = '.';
                switch (CUR(i,j)->type) {
                    case PLANT:     ch = 'P'; break;
                    case HERBIVORE: ch = 'H'; break;
                    case CARNIVORE: ch = 'C'; break;
                }
                printf("%c ", ch);
            }
            printf("\n");
        }
    }
}

/* API wrappers */
void ecosystem_init(int rows, int cols, int num_threads,
                    int n_plants, int n_herb, int n_carn) {
    ROWS = rows; COLS = cols; NUM_THREADS = num_threads;
    init_ecosystem(n_plants, n_herb, n_carn);

    seeds = malloc(NUM_THREADS * sizeof(unsigned int));
    for (int t = 0; t < NUM_THREADS; t++) seeds[t] = 1000 + t * 37;
}

void ecosystem_run(int ticks) {
    double t_total = 0.0;
    for (int tick = 1; tick <= ticks; tick++) {
        double t_start = omp_get_wtime();

        phase1_compute_intents(seeds);
        phase2_resolve_claims();
        phase3_apply(seeds);

        Cell *tmp = grid_cur;
        grid_cur  = grid_next;
        grid_next = tmp;

        double t_end = omp_get_wtime();
        t_total += (t_end - t_start);

        if (tick == 1 || tick == 5 || tick % 10 == 0) {
            print_state(tick);
            printf("  Tiempo tick %d: %.4f ms\n",
                   tick, (t_end - t_start) * 1000.0);
        }
    }

    printf("\n=== Simulación finalizada ===\n");
    printf("Tiempo total: %.4f s  |  Promedio por tick: %.4f ms\n",
           t_total, t_total / ticks * 1000.0);
}

void ecosystem_free(void) {
    free(grid_cur);
    free(grid_next);
    free(claim);
    free(intents);
    free(seeds);
    grid_cur = grid_next = NULL;
    claim = NULL;
    intents = NULL;
    seeds = NULL;
}
