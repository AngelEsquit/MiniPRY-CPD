/* Estado y helpers compartidos entre core/species/sync/io/util. Privado. */
#ifndef ECOSYSTEM_INTERNAL_H
#define ECOSYSTEM_INTERNAL_H

#include <stdlib.h>
#include <omp.h>
#include "../include/ecosystem.h"

#define EMPTY     0
#define PLANT     1
#define HERBIVORE 2
#define CARNIVORE 3

/* Reglas.pdf: probabilidades y umbrales de las reglas del ecosistema */
#define PLANT_SPREAD_PROB   0.30
#define HERB_REPRODUCE_PROB 0.25
#define CARN_REPRODUCE_PROB 0.20
#define HERB_ENERGY_INIT    3
#define CARN_ENERGY_INIT    5
#define HERB_ENERGY_EAT     1
#define CARN_ENERGY_EAT     3
#define HERB_STARVE_TICKS   3
#define CARN_STARVE_TICKS   20
#define MAX_AGE             30   /* "Muerte" del PDF también menciona vejez, usado por herbivoros */
#define CARN_MAX_AGE        60   /* separado de MAX_AGE: los carnivoros viven mas ciclos */

#define NO_MOVE -1

/* Prioridades usadas para resolver conflictos cuando dos organismos
 * quieren moverse a la misma celda destino en el mismo tick.
 * Mayor numero gana. Es una heuristica simple: cazar > huir > comer
 * planta > deambular > expandirse (planta). */
#define SCORE_PLANT_SPREAD 1
#define SCORE_WANDER       5
#define SCORE_HERB_EAT     10
#define SCORE_HERB_FLEE    15
#define SCORE_CARN_EAT     20

/* Pesos para el puntaje de CANDIDATOS a celda vecina de un herbivoro
 * (no confundir con los SCORE_* de arriba, que son la prioridad usada
 * para resolver conflictos de destino). Un herbivoro pondera cada
 * vecino valido como:
 *
 *   score(vecino) = W_FLEE * dist_to_carnivore[vecino]
 *                 - W_SEEK * dist_to_plant[vecino]
 *                 + (vecino tiene planta ? EAT_BONUS : 0)
 *
 * y se mueve al vecino con mayor score. Esto reemplaza una cascada
 * rigida de if/else (huir siempre gana sobre comer) por una
 * ponderacion real: un herbivoro lejos de peligro pero cerca de
 * comida prioriza comer; uno con un carnivoro encima prioriza huir
 * incluso si eso lo aleja de una planta. */
#define HERB_W_FLEE    5
#define HERB_W_SEEK    2
#define HERB_EAT_BONUS 20

/* Radio maximo (en pasos) hasta donde se propaga cada mapa de
 * distancias. No hace falta la distancia exacta en todo el grid: a
 * un organismo solo le importa la direccion, y limitar el radio
 * acota el costo de la relajacion paralela por tick. */
#define MAX_RELAX_RADIUS 8
#define DIST_INF 999999

/* Un movimiento propuesto por una celda origen. Cada celda escribe
 * SOLO en su propio indice (moves[IDX(i,j)]), asi que calcular esto
 * en paralelo no tiene conflictos de escritura. */
typedef struct {
    int dst_i, dst_j;
    int score;
    int valid;
} Move;

/* Estado global de la simulación (definido en core/grid.c) */
extern int   ROWS, COLS, NUM_THREADS;
extern Cell *grid_cur;
extern Cell *grid_next;
extern unsigned int *seeds;

/* Movelist + resolución de conflictos (movelist + locks, ver diseño) */
extern Move        *moves;       /* una propuesta por celda origen */
extern int         *claim_score; /* mejor score que reclama cada destino */
extern int         *claim_src;   /* quien es el dueño actual de cada destino */
extern omp_lock_t  *cell_locks;  /* un lock por celda destino */

/* moved[k] = 1 si la celda origen k gano su movimiento Y ese
 * movimiento implica abandonar la celda (herbivoro/carnivoro que se
 * va). Para plantas NO se marca: expandirse no vacia la celda madre,
 * la planta original se queda donde estaba. Sin esto, un
 * herbivoro/carnivoro que se movio apareceria duplicado: una vez en
 * su celda nueva y otra vez "quieto" en la vieja. */
extern int *moved;

/* eaten[k] = 1 si la celda origen k (un herbivoro) fue cazada este
 * tick por un carnivoro que gano el reclamo sobre su propia celda de
 * origen. Un herbivoro puede, en el mismo tick, tambien haber ganado
 * SU propio movimiento hacia otra celda -- son dos resoluciones de
 * destino independientes (celdas distintas), asi que nada las liga
 * automaticamente. Sin este chequeo, el herbivoro cazado igual
 * aparaceria vivo en su celda destino. apply.c usa esto para no
 * colocar a un mover cuyo origen ya fue comido. */
extern int *eaten;

/* Cola de nacimientos por reproduccion. Un herbivoro/carnivoro que se
 * mueve y come puede reproducirse dejando una cria en la celda que
 * acaba de desocupar. Como esa celda tambien se procesa en paralelo
 * como destino de otra celda (posiblemente en otro hilo, en la misma
 * pasada), escribirla directamente ahi seria una carrera de datos.
 * En cambio, cada nacimiento reserva un slot con un contador atomico
 * (#pragma omp atomic capture) y se aplica en una pasada aparte, ya
 * con la pasada principal terminada (barrier implicito de por medio). */
typedef struct { int i, j, type; } Birth;
extern Birth *births;
extern int birth_count;

/* Mapas de distancia (recalculados cada tick desde grid_cur) */
extern int *dist_to_plant;
extern int *dist_to_herbivore;
extern int *dist_to_carnivore;

extern const int DR[4];
extern const int DC[4];

static inline int   IDX(int i, int j)  { return i * COLS + j; }
static inline Cell *CUR(int i, int j)  { return &grid_cur [IDX(i,j)]; }
static inline Cell *NEXT(int i, int j) { return &grid_next[IDX(i,j)]; }
static inline unsigned int next_rand_u32(unsigned int *seed) {
    // LCG simple y portable para evitar dependencia de rand_r en Windows/UCRT.
    *seed = (*seed * 1103515245u) + 12345u;
    return *seed;
}
static inline double rand_d(unsigned int *seed) {
    return (double)next_rand_u32(seed) / 4294967295.0;
}
static inline int in_bounds(int i, int j) {
    return i >= 0 && i < ROWS && j >= 0 && j < COLS;
}

/* util/rng.c */
void shuffle(int *arr, int n, unsigned int *seed);

/* core/grid.c */
void grid_init(int n_plants, int n_herb, int n_carn);
void grid_destroy(void);

/* core/distance_map.c: distancia (en pasos) a la celda mas cercana de seed_type */
void compute_distance_map(int seed_type, int *out);

/* species/plants.c, species/herbivores.c, species/carnivores.c */
void decide_plant_moves(unsigned int *seeds_local);
void decide_herbivore_moves(unsigned int *seeds_local);
void decide_carnivore_moves(unsigned int *seeds_local);

/* sync/move_resolve.c */
void reset_moves(void);
void resolve_moves(void);

/* core/apply.c */
void apply_moves(unsigned int *seeds_local);

/* io/print_state.c */
void print_state(int tick);
void print_state_live(int tick);

#endif /* ECOSYSTEM_INTERNAL_H */
