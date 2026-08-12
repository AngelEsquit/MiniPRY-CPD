/*
 * core/apply.c
 * Paso 4 del pipeline. Con claim_src[]/moved[]/eaten[] ya resueltos,
 * esta pasada llena grid_next: cada hilo procesa una celda destino
 * distinta, asi que no hay conflicto de escritura sobre grid_next en
 * si. Aca es donde se aplica comer, morir y encolar nacimientos.
 *
 * La unica escritura compartida real que queda es la de reproduccion:
 * una cria se coloca en la celda que su padre acaba de desocupar, y
 * esa celda TAMBIEN se procesa como destino en esta misma pasada (por
 * otro hilo, en otra iteracion). Escribirla directamente ahi seria una
 * carrera. En cambio se encola con un contador atomico (birth_count) y
 * se aplica en una pasada aparte, despues del barrier implicito que
 * cierra el primer parallel for.
 */
#include "../internal.h"
#include <omp.h>

// Helper compartido: decide si el organismo sigue vivo o muere este tick
static void apply_death_or_survive(int i, int j, Cell new_c, int starve_limit,
                                   int age_limit) {
  if (new_c.energy <= 0 || new_c.starve >= starve_limit ||
      new_c.age >= age_limit) {
    NEXT(i, j)[0] = (Cell){EMPTY, 0, 0, 0}; // murio: celda destino vacia
  } else {
    NEXT(i, j)[0] = new_c; // sobrevive: se escribe con su estado actualizado
  }
}

void apply_moves(unsigned int *seeds_local) {
  int total = ROWS * COLS;

// Inicializar grid_next en vacio antes de escribir nada
#pragma omp parallel for
  for (int k = 0; k < total; k++)
    grid_next[k] = (Cell){EMPTY, 0, 0, 0};

// schedule dynamic: el trabajo por celda varia mucho (celda vacia
// vs. celda con un organismo que hay que envejecer/alimentar/matar)
#pragma omp parallel for collapse(2) schedule(dynamic, 16)
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int tid = omp_get_thread_num();
      unsigned int *seed = &seeds_local[tid];

      int dst = IDX(i, j);
      int src = claim_src[dst];
      Cell *dst_cur = CUR(i, j);

      if (src == NO_MOVE) {
        // Nadie gano esta celda como destino: se queda como
        // estaba, o vacia si el ocupante se fue este tick
        if (moved[dst])
          continue;
        if (dst_cur->type == EMPTY)
          continue;

        Cell new_c = *dst_cur;
        new_c.age++; // envejecer un tick

        if (new_c.type == PLANT) {
          NEXT(i, j)[0] = new_c; // las plantas no pasan hambre
          continue;
        }

        // Quieto: suma un tick de hambre (sin gastar energia,
        // solo el conteo de starve mata por inanicion)
        new_c.starve++;
        int starve_limit =
            (new_c.type == HERBIVORE) ? HERB_STARVE_TICKS : CARN_STARVE_TICKS;
        int age_limit = (new_c.type == HERBIVORE) ? MAX_AGE : CARN_MAX_AGE;
        apply_death_or_survive(i, j, new_c, starve_limit, age_limit);
        continue;
      }

      // Cazado en su propia celda este tick: no colocar en destino
      if (eaten[src])
        continue;

      // Traer el estado del organismo que gano este destino
      int si = src / COLS, sj = src % COLS;
      Cell new_c = *CUR(si, sj);
      new_c.age++;

      if (new_c.type == PLANT) {
        // Expandirse crea un brote nuevo; la planta madre sigue
        // en (si,sj), moved[] nunca la marca
        NEXT(i, j)[0] = (Cell){PLANT, 0, 0, 0};
        continue;
      }

      if (new_c.type == HERBIVORE) {
        if (dst_cur->type == PLANT) {
          // Come: gana energia, resetea hambre
          new_c.energy += HERB_ENERGY_EAT;
          new_c.starve = 0;
          if (new_c.energy >= HERB_ENERGY_INIT * 2 &&
              rand_d(seed) < HERB_REPRODUCE_PROB) {
            // Reproduce: resta el costo de energia y encola
            // la cria (no se coloca aca, ver el loop de abajo)
            new_c.energy -= HERB_ENERGY_INIT;
            int idx;
#pragma omp atomic capture
            idx = birth_count++; // reservar slot unico en births[]
            births[idx] = (Birth){si, sj, HERBIVORE};
          }
        } else {
          // Se movio sin comer: solo suma un tick de hambre
          new_c.starve++;
        }
        apply_death_or_survive(i, j, new_c, HERB_STARVE_TICKS, MAX_AGE);
        continue;
      }

      // CARNIVORE
      if (dst_cur->type == HERBIVORE) {
        // Cazo: gana energia, resetea hambre
        new_c.energy += CARN_ENERGY_EAT;
        new_c.starve = 0;
        if (new_c.energy >= CARN_ENERGY_INIT * 2 &&
            rand_d(seed) < CARN_REPRODUCE_PROB) {
          new_c.energy -= CARN_ENERGY_INIT;
          int idx;
#pragma omp atomic capture
          idx = birth_count++;
          births[idx] = (Birth){si, sj, CARNIVORE};
        }
      } else {
        // Se movio sin cazar
        new_c.starve++;
      }
      apply_death_or_survive(i, j, new_c, CARN_STARVE_TICKS, CARN_MAX_AGE);
    }
  }

// Colocar crias encoladas: cada una apunta a un origen distinto
// (un padre se reproduce a lo sumo una vez por tick), sin conflicto
#pragma omp parallel for
  for (int b = 0; b < birth_count; b++) {
    int bi = births[b].i, bj = births[b].j;
    if (NEXT(bi, bj)->type ==
        EMPTY) { // por si un depredador tomo esa celda igual
      int init_energy =
          (births[b].type == HERBIVORE) ? HERB_ENERGY_INIT : CARN_ENERGY_INIT;
      NEXT(bi, bj)[0] = (Cell){births[b].type, init_energy, 0, 0};
    }
  }
}
