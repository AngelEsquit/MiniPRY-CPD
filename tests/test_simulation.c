/*
 * tests/test_simulation.c
 * Pruebas unitarias e integrales para la simulación de ecosistema OpenMP.
 * Valida la inicialización, la invariante de datos en cada celda y la ejecución limpia.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ecosystem.h"
#include "../src/internal.h"

static void test_initialization(void) {
    printf("[TEST] Inicialización del ecosistema...");
    int rows = 20, cols = 20, threads = 2;
    int plants = 30, herb = 10, carn = 10;
    
    ecosystem_init(rows, cols, threads, plants, herb, carn);

    assert(ROWS == rows);
    assert(COLS == cols);
    assert(NUM_THREADS == threads);
    assert(grid_cur != NULL);
    assert(grid_next != NULL);

    int count_p = 0, count_h = 0, count_c = 0, count_e = 0;
    for (int k = 0; k < rows * cols; k++) {
        switch (grid_cur[k].type) {
            case PLANT:     count_p++; break;
            case HERBIVORE: count_h++; break;
            case CARNIVORE: count_c++; break;
            case EMPTY:     count_e++; break;
            default:
                assert(0 && "Tipo de celda inválido en la cuadrícula inicial");
        }
    }

    assert(count_p == plants);
    assert(count_h == herb);
    assert(count_c == carn);
    assert(count_e == (rows * cols - plants - herb - carn));

    ecosystem_free();
    printf(" OK\n");
}

static void test_simulation_ticks(void) {
    printf("[TEST] Ejecución de ticks e invariantes del sistema...");
    int rows = 15, cols = 15, threads = 4;
    int plants = 20, herb = 10, carn = 5;

    ecosystem_init(rows, cols, threads, plants, herb, carn);
    ecosystem_run(10, 0); // Ejecutar 10 ticks sin animación live

    // Verificar invariantes sobre todas las celdas tras 10 ticks
    for (int k = 0; k < rows * cols; k++) {
        int t = grid_cur[k].type;
        assert(t == EMPTY || t == PLANT || t == HERBIVORE || t == CARNIVORE);
        assert(grid_cur[k].energy >= 0);
        assert(grid_cur[k].starve >= 0);
        assert(grid_cur[k].age >= 0);
    }

    ecosystem_free();
    printf(" OK\n");
}

int main(void) {
    printf("=== Ejecutando Suite de Pruebas del Ecosistema ===\n");
    test_initialization();
    test_simulation_ticks();
    printf("=== Todas las pruebas pasaron exitosamente ===\n");
    return 0;
}
