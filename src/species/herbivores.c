/*
 * species/herbivores.c
 * Paso 2b del pipeline. Cada herbivoro pondera sus hasta 4 celdas
 * vecinas transitables (vacias, o con una planta que puede comer) con
 * una combinacion de que tan lejos quedaria del carnivoro mas cercano
 * y que tan cerca quedaria de la planta mas cercana, mas un bono si la
 * celda misma tiene una planta (HERB_W_FLEE / HERB_W_SEEK /
 * HERB_EAT_BONUS en internal.h). Se mueve al vecino con mayor puntaje.
 *
 * Igual que species/plants.c: cada celda escribe SOLO su propio
 * moves[IDX(i,j)], no hay conflicto de escritura aca.
 */

#include "../internal.h"
#include <omp.h>

void decide_herbivore_moves(unsigned int *seeds_local) {
// schedule dynamic por la misma razon que en plants.c: la mayoria
// de celdas no son herbivoros y se descartan de inmediato, asi que
// el trabajo real esta concentrado y desbalanceado por el grid
#pragma omp parallel for collapse(2) schedule(dynamic, 16)
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int tid = omp_get_thread_num();
      unsigned int *seed = &seeds_local[tid];

      if (CUR(i, j)->type != HERBIVORE)
        continue;
      Move *mv = &moves[IDX(i, j)];

      // Orden de vecinos al azar (para no favorecer siempre la
      // misma direccion en un empate de puntaje)
      int order[4] = {0, 1, 2, 3};
      shuffle(order, 4, seed);

      // Variables acumuladoras: mejor vecino visto hasta ahora
      int best_i = -1, best_j = -1, best_has_plant = 0;
      long best_score = 0;
      int found = 0;

      // Puntuar cada vecino transitable: mas lejos del carnivoro
      // mas cercano suma, mas cerca de una planta suma, comer ahi
      // mismo suma un bono fijo
      for (int d = 0; d < 4; d++) {
        int ni = i + DR[order[d]], nj = j + DC[order[d]];
        if (!in_bounds(ni, nj))
          continue;

        int ntype = CUR(ni, nj)->type;
        if (ntype != EMPTY && ntype != PLANT)
          continue; // solo vacio o comida

        long score = (long)HERB_W_FLEE * dist_to_carnivore[IDX(ni, nj)] -
                     (long)HERB_W_SEEK * dist_to_plant[IDX(ni, nj)];
        if (ntype == PLANT)
          score += HERB_EAT_BONUS;

        // Actualizar el mejor candidato si este vecino puntua mas alto
        if (!found || score > best_score) {
          found = 1;
          best_score = score;
          best_i = ni;
          best_j = nj;
          best_has_plant = (ntype == PLANT);
        }
      }

      if (!found)
        continue; // rodeado, se queda quieto

      // Guardar el ganador en moves[]
      mv->dst_i = best_i;
      mv->dst_j = best_j;
      mv->valid = 1;

      // Prioridad de conflicto (para sync/move_resolve.c), no
      // confundir con el score de arriba que solo elige el vecino
      if (best_has_plant) {
        mv->score = SCORE_HERB_EAT;
      } else if (dist_to_carnivore[IDX(i, j)] <= 1) {
        mv->score = SCORE_HERB_FLEE;
      } else {
        mv->score = SCORE_WANDER;
      }
    }
  }
}
