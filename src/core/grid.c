/*
 * core/grid.c
 * Paso 0 del pipeline (antes de que arranque cualquier tick). Reserva,
 * inicializa y libera todo el estado global declarado como extern en
 * internal.h.
 */
#include "../internal.h"
#include <stdio.h>
#include <stdlib.h>

// Definicion real de las variables globales declaradas extern en internal.h
int ROWS, COLS, NUM_THREADS;
Cell *grid_cur = NULL;
Cell *grid_next = NULL;
unsigned int *seeds = NULL;

Move *moves = NULL;
int *claim_score = NULL;
int *claim_src = NULL;
omp_lock_t *cell_locks = NULL;
int *moved = NULL;
int *eaten = NULL;
Birth *births = NULL;
int birth_count = 0;

int *dist_to_plant = NULL;
int *dist_to_herbivore = NULL;
int *dist_to_carnivore = NULL;

// Inicialización del grid
void grid_init(int n_plants, int n_herb, int n_carn) {
  int total = ROWS * COLS;

  // Alocaciones de memoria, un slot por celda en cada arreglo auxiliar
  grid_cur = calloc(total, sizeof(Cell));
  grid_next = calloc(total, sizeof(Cell));
  moves = malloc(total * sizeof(Move));
  claim_score = malloc(total * sizeof(int));
  claim_src = malloc(total * sizeof(int));
  cell_locks = malloc(total * sizeof(omp_lock_t));
  moved = malloc(total * sizeof(int));
  eaten = malloc(total * sizeof(int));
  births = malloc(total * sizeof(Birth));

  dist_to_plant = malloc(total * sizeof(int));
  dist_to_herbivore = malloc(total * sizeof(int));
  dist_to_carnivore = malloc(total * sizeof(int));

  // Si cualquier alocacion fallo, no seguir con punteros invalidos
  if (!grid_cur || !grid_next || !moves || !claim_score || !claim_src ||
      !cell_locks || !moved || !eaten || !births || !dist_to_plant ||
      !dist_to_herbivore || !dist_to_carnivore) {
    fprintf(stderr, "Error: sin memoria\n");
    exit(1);
  }

  // Un lock de OpenMP por celda, usado despues en sync/move_resolve.c
  for (int k = 0; k < total; k++)
    omp_init_lock(&cell_locks[k]);

  // Barajar todos los indices de la grilla para repartir la poblacion
  // inicial al azar, sin que dos especies puedan caer en la misma celda
  int *indices = malloc(total * sizeof(int));
  for (int i = 0; i < total; i++)
    indices[i] = i;

  unsigned int seed =
      42; // semilla fija: la disposicion inicial es reproducible
  shuffle(indices, total, &seed);

  // Colocar plantas, luego herbivoros, luego carnivoros en indices ya barajados
  int placed = 0;
  for (int i = 0; i < n_plants && placed < total; i++, placed++) {
    grid_cur[indices[placed]] = (Cell){PLANT, 0, 0, 0};
  }
  for (int i = 0; i < n_herb && placed < total; i++, placed++) {
    grid_cur[indices[placed]] = (Cell){HERBIVORE, HERB_ENERGY_INIT, 0, 0};
  }
  for (int i = 0; i < n_carn && placed < total; i++, placed++) {
    grid_cur[indices[placed]] = (Cell){CARNIVORE, CARN_ENERGY_INIT, 0, 0};
  }

  free(indices); // solo se necesitaba para la colocacion inicial
}

void grid_destroy(void) {
  int total = ROWS * COLS;

  // Liberar los locks de OpenMP antes de liberar el arreglo que los contiene
  for (int k = 0; k < total; k++)
    omp_destroy_lock(&cell_locks[k]);

  free(grid_cur);
  free(grid_next);
  free(moves);
  free(claim_score);
  free(claim_src);
  free(cell_locks);
  free(moved);
  free(eaten);
  free(births);
  free(dist_to_plant);
  free(dist_to_herbivore);
  free(dist_to_carnivore);

  grid_cur = grid_next = NULL;
  moves = NULL;
  claim_score = claim_src = NULL;
  cell_locks = NULL;
  moved = NULL;
  eaten = NULL;
  births = NULL;
  dist_to_plant = dist_to_herbivore = dist_to_carnivore = NULL;
}
