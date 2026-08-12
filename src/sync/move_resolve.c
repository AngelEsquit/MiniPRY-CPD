/*
 * sync/move_resolve.c
 * Paso 3 del pipeline. Resuelve conflictos de destino: si dos o mas
 * celdas origen (de cualquier especie) proponen la misma celda
 * destino en moves[], aca se decide cual gana.
 *
 * reset_moves(): limpia moves[], claim_src[], claim_score[], moved[]
 * y eaten[] al arrancar el tick. Cada indice pertenece a un solo hilo
 * (misma logica que grid_cur/grid_next), asi que no hace falta lock aca.
 *
 * resolve_moves(): aca es donde SI puede haber conflicto real. Se
 * resuelve con un lock por celda DESTINO: cada hilo bloquea solo la
 * celda a la que quiere escribir, compara su score contra quien la
 * tiene reclamada, y se queda con el que gane. Como cada lock protege
 * una sola celda, dos hilos que van a destinos distintos nunca se
 * bloquean entre si.
 */
#include "../internal.h"
#include <omp.h>

void reset_moves(void) {
  int total = ROWS * COLS;
// Cada indice pertenece a un solo hilo, no hace falta lock aca
#pragma omp parallel for
  for (int k = 0; k < total; k++) {
    moves[k].valid = 0;     // sin propuesta todavia este tick
    claim_src[k] = NO_MOVE; // nadie reclama esta celda todavia
    claim_score[k] = -1;    // cualquier score real le gana a esto
    moved[k] = 0;
    eaten[k] = 0;
  }
  birth_count = 0; // vaciar la cola de nacimientos del tick anterior
}

void resolve_moves(void) {
  int total = ROWS * COLS;

// Conflicto: dos origenes distintos proponiendo el mismo
// destino. Solo se bloquean entre si los que compiten por la misma celda.
// schedule dynamic porque la mayoria de entradas de moves[] son invalidas
// (celda vacia u organismo que no se movio) y se descartan de inmediato.
#pragma omp parallel for schedule(dynamic, 16)
  for (int k = 0; k < total; k++) {
    if (!moves[k].valid)
      continue; // esta celda no propuso nada

    int dst = IDX(moves[k].dst_i, moves[k].dst_j);

    // Comparar-y-actualizar protegido por el lock de esa celda destino
    omp_set_lock(&cell_locks[dst]);
    if (moves[k].score > claim_score[dst]) {
      claim_score[dst] = moves[k].score;
      claim_src[dst] = k;
    }
    omp_unset_lock(&cell_locks[dst]);
  }

// Marcar quien realmente abandona su celda (para no duplicarlo
// quieto en core/apply.c). Cada origen gana a lo sumo un destino,
// asi que cada iteracion escribe un moved[] distinto: sin carrera.
#pragma omp parallel for
  for (int dst = 0; dst < total; dst++) {
    int src = claim_src[dst];
    if (src != NO_MOVE && grid_cur[src].type != PLANT) {
      moved[src] = 1;
    }
  }

// Marcar herbivoros cazados en su propia celda de origen: el
// ganador de dst es un carnivoro y dst tenia un herbivoro -> dst
// ES el indice de origen del herbivoro comido.
#pragma omp parallel for
  for (int dst = 0; dst < total; dst++) {
    int src = claim_src[dst];
    if (src != NO_MOVE && grid_cur[src].type == CARNIVORE &&
        grid_cur[dst].type == HERBIVORE) {
      eaten[dst] = 1;
    }
  }
}
