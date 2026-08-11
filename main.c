#include <stdio.h>
#include <stdlib.h>
#include "ecosystem.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,
            "Uso: %s <filas> <cols> <ticks> <hilos>\n"
            "  Ejemplo: %s 20 20 50 8\n", argv[0], argv[0]);
        return 1;
    }

    int ROWS      = atoi(argv[1]);
    int COLS      = atoi(argv[2]);
    int ticks     = atoi(argv[3]);
    int NUM_THR   = atoi(argv[4]);

    int total     = ROWS * COLS;
    int n_plants  = total * 0.30;
    int n_herb    = total * 0.10;
    int n_carn    = total * 0.05;

    printf("=== Simulación de Ecosistema con OpenMP ===\n");
    printf("Grid: %dx%d  |  Ticks: %d  |  Hilos: %d\n",
           ROWS, COLS, ticks, NUM_THR);
    printf("Inicial → Plantas: %d | Herbívoros: %d | Carnívoros: %d\n",
           n_plants, n_herb, n_carn);

    ecosystem_init(ROWS, COLS, NUM_THR, n_plants, n_herb, n_carn);
    ecosystem_run(ticks);
    ecosystem_free();

    return 0;
}