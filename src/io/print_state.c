/*
 * io/print_state.c
 * No es parte del pipeline de simulacion (que termina en core/apply.c),
 * es solo la capa de reporte que llama core/simulation.c despues de
 * cada tick: print_state (snapshot periodico) o print_state_live
 * (animacion coloreada con --live).
 */
#include <stdio.h>
#include <omp.h>
#include "../internal.h"

// Contar poblacion por tipo, en paralelo con reduccion
static void count_population(int *plants, int *herb, int *carn) {
    int p = 0, h = 0, c = 0;
    #pragma omp parallel for reduction(+:p,h,c)
    for (int k = 0; k < ROWS * COLS; k++) {
        if (grid_cur[k].type == PLANT)     p++;
        if (grid_cur[k].type == HERBIVORE) h++;
        if (grid_cur[k].type == CARNIVORE) c++;
    }
    *plants = p; *herb = h; *carn = c;
}

void print_state(int tick) {
    int plants, herb, carn;
    count_population(&plants, &herb, &carn);

    printf("\n** Tick %d **\n", tick);
    printf("Plantas: %d | Herbívoros: %d | Carnívoros: %d\n",
           plants, herb, carn);

    // Solo dibujar el mapa de caracteres si entra comodo en pantalla
    if (ROWS <= 30 && COLS <= 30) {
        printf("Distribución:\n");
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                char ch = '.';
                switch (CUR(i,j)->type) {
                    case PLANT:     ch = 'P'; break;
                    case HERBIVORE: ch = 'H'; break;
                    case CARNIVORE: ch = 'C'; break;
                }
                printf("%c ", ch);
            }
            printf("\n");
        }
    }
}

// Colores ANSI por especie, celda vacia sin color
#define COLOR_PLANT     "\033[32mP\033[0m" // verde
#define COLOR_HERBIVORE "\033[33mH\033[0m" // amarillo
#define COLOR_CARNIVORE "\033[31mC\033[0m" // rojo
#define COLOR_EMPTY     "."

void print_state_live(int tick) {
    int plants, herb, carn;
    count_population(&plants, &herb, &carn);

    printf("\033[H\033[2J"); // cursor a home + limpiar pantalla, redibuja el frame
    printf("=== Tick %d ===\n", tick);
    printf("Plantas: %d | Herbívoros: %d | Carnívoros: %d\n\n",
           plants, herb, carn);

    // Dibujar la grilla completa (sin limite de tamano, a diferencia de print_state)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            switch (CUR(i,j)->type) {
                case PLANT:     printf(COLOR_PLANT " ");     break;
                case HERBIVORE: printf(COLOR_HERBIVORE " "); break;
                case CARNIVORE: printf(COLOR_CARNIVORE " "); break;
                default:        printf(COLOR_EMPTY " ");     break;
            }
        }
        printf("\n");
    }
    fflush(stdout); // forzar el frame a pantalla ya, sin esperar buffer
}
