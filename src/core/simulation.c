/*
 * core/simulation.c
 * Orquestador del pipeline. Implementa la API publica de
 * include/ecosystem.h: ecosystem_init reserva el estado, ecosystem_run
 * corre el ciclo de tick a tick, ecosystem_free libera todo.
 *
 * Orden de un tick (cada paso es su propio archivo):
 *   1. core/distance_map.c   -> mapas de distancia desde grid_cur
 *   2. species/*.c           -> cada especie propone un movimiento
 *   3. sync/move_resolve.c   -> resuelve conflictos de destino
 *   4. core/apply.c          -> escribe grid_next
 *   5. swap grid_cur <-> grid_next (doble buffer)
 */
#include "../internal.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LIVE_FRAME_DELAY_US                                                    \
  80000 // ~12 fps, para que se pueda seguir a simple vista

void ecosystem_init(int rows, int cols, int num_threads, int n_plants,
                    int n_herb, int n_carn) {
  // Guardar dimensiones globales, las usa todo el resto del codigo via
  // internal.h
  ROWS = rows;
  COLS = cols;
  NUM_THREADS = num_threads;
  omp_set_num_threads(NUM_THREADS);

  // reservar y puebla grid_cur/next y demas arreglos
  grid_init(n_plants, n_herb, n_carn);

  // Una semilla de RNG por hilo, para que nadie comparta estado de rand_r
  seeds = malloc(NUM_THREADS * sizeof(unsigned int));
  for (int t = 0; t < NUM_THREADS; t++)
    seeds[t] = 1000 + t * 37;
}

void ecosystem_run(int ticks, int live) {
  double t_total = 0.0;

  for (int tick = 1; tick <= ticks; tick++) {
    double t_start = omp_get_wtime();

    // Mapas de distancia desde el estado congelado
    compute_distance_map(PLANT, dist_to_plant);
    compute_distance_map(HERBIVORE, dist_to_herbivore);
    compute_distance_map(CARNIVORE, dist_to_carnivore);

    // Cada especie propone su movimiento (moves[], sin conflictos)
    reset_moves();
    decide_plant_moves(seeds);
    decide_herbivore_moves(seeds);
    decide_carnivore_moves(seeds);

    // Resolver conflictos de destino
    resolve_moves();

    // Aplicar el resultado a grid_next
    apply_moves(seeds);

    // grid_next = grid_cur para actualizar el estado
    Cell *tmp = grid_cur;
    grid_cur = grid_next;
    grid_next = tmp;

    double t_end = omp_get_wtime();
    t_total += (t_end - t_start);

    // Print / reporte por tick
    if (live) {
      print_state_live(tick);
      usleep(LIVE_FRAME_DELAY_US);
    } else if (tick == 1 || tick == 5 || tick % 10 == 0) {
      print_state(tick);
      printf("  Tiempo tick %d: %.4f ms\n", tick, (t_end - t_start) * 1000.0);
    }
  }

  printf("\n=== Simulación finalizada ===\n");
  printf("Tiempo total: %.4f s  |  Promedio por tick: %.4f ms\n", t_total,
         t_total / ticks * 1000.0);
}

void ecosystem_free(void) {
  grid_destroy(); // libera grid_cur/next y todos los arreglos auxiliares
  free(seeds);
  seeds = NULL;
}
