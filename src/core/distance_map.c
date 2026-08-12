/*
 * core/distance_map.c
 * Paso 1 del pipeline. Calcula, para cada celda, la distancia (en
 * pasos) a la celda mas cercana de un tipo dado (planta / herbivoro /
 * carnivoro). Es un BFS multi-fuente hecho con relajacion paralela por
 * rondas en vez de una cola compartida (que seria otro punto de
 * sincronizacion): cada celda solo mira a sus 4 vecinos por ronda.
 *
 * Doble buffer, cada ronda LEE de un array y ESCRIBE en otro. Ningun
 * hilo escribe donde otro esta leyendo, asi que no hace falta lock ni
 * atomic aca; el unico punto de sincronizacion es el barrier implicito
 * al final de cada "#pragma omp parallel for" (ronda).
 */

#include "../internal.h"
#include <omp.h>
#include <stdlib.h>

void compute_distance_map(int seed_type, int *out) {
  int total = ROWS * COLS;
  int *buf_a = malloc(total * sizeof(int)); // lo que se lee esta ronda
  int *buf_b = malloc(total * sizeof(int)); // lo que se escribe esta ronda

// 0 en las fuentes (celdas del tipo buscado), infinito en el resto
#pragma omp parallel for
  for (int k = 0; k < total; k++) {
    buf_a[k] = (grid_cur[k].type == seed_type) ? 0 : DIST_INF;
  }

  for (int round = 0; round < MAX_RELAX_RADIUS; round++) {
// Cada celda toma el minimo entre su propia distancia
// y la de sus vecinos + 1. collapse(2) junta las dos dimensiones
// del grid en un solo espacio de iteracion para repartir entre hilos
#pragma omp parallel for collapse(2)
    for (int i = 0; i < ROWS; i++) {
      for (int j = 0; j < COLS; j++) {
        int best = buf_a[IDX(i, j)];
        for (int d = 0; d < 4; d++) {
          int ni = i + DR[d], nj = j + DC[d];
          if (!in_bounds(ni, nj))
            continue;
          int cand = buf_a[IDX(ni, nj)] + 1;
          if (cand < best)
            best = cand;
        }
        buf_b[IDX(i, j)] = best;
      }
    }

    // Swap: lo que acabamos de escribir es la foto de partida de la
    // proxima ronda
    int *tmp = buf_a;
    buf_a = buf_b;
    buf_b = tmp;
  }

// Copiar el resultado final (queda en buf_a tras el ultimo swap) a out[]
#pragma omp parallel for
  for (int k = 0; k < total; k++)
    out[k] = buf_a[k];

  free(buf_a);
  free(buf_b);
}
