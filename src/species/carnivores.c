/*
 * species/carnivores.c
 * Paso 2c del pipeline. Reglas para carnivoros: cazan un herbivoro
 * adyacente si hay uno, si no caminan hacia el herbivoro mas cercano
 * usando el mapa de distancias, si no hay ninguno cerca deambulan.
 */
#include "../internal.h"
#include <omp.h>

void decide_carnivore_moves(unsigned int *seeds_local) {
// schedule dynamic: misma razon que en las otras dos especies,
// los carnivoros son la minoria de celdas y quedan repartidos de
// forma pareja solo si el trabajo se reparte en runtime
#pragma omp parallel for collapse(2) schedule(dynamic, 16)
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int tid = omp_get_thread_num();
      unsigned int *seed = &seeds_local[tid];

      if (CUR(i, j)->type != CARNIVORE)
        continue;
      Move *mv = &moves[IDX(i, j)];

      int order[4] = {0, 1, 2, 3};
      shuffle(order, 4, seed);

      // Cazar si hay un herbivoro adyacente
      int hunted = 0;
      for (int d = 0; d < 4; d++) {
        int ni = i + DR[order[d]], nj = j + DC[order[d]];
        if (!in_bounds(ni, nj))
          continue;
        if (CUR(ni, nj)->type == HERBIVORE) {
          // Guardar la caza como movimiento propuesto
          mv->dst_i = ni;
          mv->dst_j = nj;
          mv->score = SCORE_CARN_EAT;
          mv->valid = 1;
          hunted = 1;
          break;
        }
      }
      if (hunted)
        continue; // ya decidido, no seguir evaluando

      // Caminar hacia el herbivoro mas cercano segun
      // el mapa de distancias (buscar, entre vecinos vacios, el
      // que minimice dist_to_herbivore)
      int self_dist = dist_to_herbivore[IDX(i, j)];
      int best_dist = self_dist, best_i = -1, best_j = -1;
      for (int d = 0; d < 4; d++) {
        int ni = i + DR[order[d]], nj = j + DC[order[d]];
        if (!in_bounds(ni, nj))
          continue;
        if (CUR(ni, nj)->type != EMPTY)
          continue;
        if (dist_to_herbivore[IDX(ni, nj)] < best_dist) {
          best_dist = dist_to_herbivore[IDX(ni, nj)];
          best_i = ni;
          best_j = nj;
        }
      }
      // Nada mejor cerca, deambular a cualquier vecino vacio
      if (best_i == -1) {
        for (int d = 0; d < 4; d++) {
          int ni = i + DR[order[d]], nj = j + DC[order[d]];
          if (!in_bounds(ni, nj))
            continue;
          if (CUR(ni, nj)->type == EMPTY) {
            best_i = ni;
            best_j = nj;
            break;
          }
        }
      }
      // Guardar el movimiento elegido, si se encontro alguno
      if (best_i != -1) {
        mv->dst_i = best_i;
        mv->dst_j = best_j;
        mv->score = SCORE_WANDER;
        mv->valid = 1;
      }
    }
  }
}
