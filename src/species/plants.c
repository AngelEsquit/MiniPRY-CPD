/*
 * species/plants.c
 * Paso 2a del pipeline (de 3, junto con herbivores.c y carnivores.c).
 * Cada planta, con 30% de probabilidad, se expande a una celda vacia adyacente
 * elegida al azar. Solo escribe moves[IDX(i,j)] (su propio indice) -> sin
 * conflictos de escritura aca; el conflicto real (dos organismos
 * queriendo la misma celda destino) se resuelve despues en
 * sync/move_resolve.c.
 */

#include "../internal.h"
#include <omp.h>

void decide_plant_moves(unsigned int *seeds_local) {
// Parallel for con schedule dynamic: la mayoria de celdas son EMPTY
// y se saltan en una linea, mientras que una celda con planta hace
// mas trabajo (tirada + shuffle + scan de vecinos).
#pragma omp parallel for collapse(2) schedule(dynamic, 16)
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      // Semilla de RNG propia del hilo, para no competir por estado global
      int tid = omp_get_thread_num();
      unsigned int *seed = &seeds_local[tid];
      Move *mv = &moves[IDX(i, j)];

      if (CUR(i, j)->type != PLANT)
        continue; // celda sin planta, nada que decidir
      if (rand_d(seed) >= PLANT_SPREAD_PROB)
        continue; // tirada de expansion

      // Elegir un vecino vacio al azar (orden de vecinos barajado)
      int order[4] = {0, 1, 2, 3};
      shuffle(order, 4, seed);
      for (int d = 0; d < 4; d++) {
        int ni = i + DR[order[d]], nj = j + DC[order[d]];
        if (!in_bounds(ni, nj))
          continue;
        if (CUR(ni, nj)->type == EMPTY) {
          // Guardar la propuesta de movimiento en moves[]
          mv->dst_i = ni;
          mv->dst_j = nj;
          mv->score = SCORE_PLANT_SPREAD;
          mv->valid = 1;
          break;
        }
      }
    }
  }
}
