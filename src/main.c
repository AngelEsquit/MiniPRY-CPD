/*
 * main.c
 * Punto de entrada. Parsea los argumentos de linea de comandos y llama
 * a la API publica (ecosystem_init -> ecosystem_run -> ecosystem_free)
 * declarada en include/ecosystem.h. No conoce nada del pipeline interno.
 */
#include "../include/ecosystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Validar argumentos minimos antes de tocar nada
  if (argc < 5) {
    fprintf(stderr,
            "Uso: %s <filas> <cols> <ticks> <hilos> [--live]\n"
            "  Ejemplo: %s 20 20 50 8\n"
            "  Ejemplo (animado): %s 40 40 200 4 --live\n",
            argv[0], argv[0], argv[0]);
    return 1;
  }

  // Leer parametros de la simulacion
  int ROWS = atoi(argv[1]);
  int COLS = atoi(argv[2]);
  int ticks = atoi(argv[3]);
  int NUM_THR = atoi(argv[4]);
  int live = (argc >= 6 && strcmp(argv[5], "--live") == 0); // flag opcional

  // Poblacion inicial como porcentaje del tamano del grid
  int total = ROWS * COLS;
  int n_plants = total * 0.30;
  int n_herb = total * 0.10;
  int n_carn = total * 0.10;

  printf("=== Simulación de Ecosistema con OpenMP ===\n");
  printf("Grid: %dx%d  |  Ticks: %d  |  Hilos: %d\n", ROWS, COLS, ticks,
         NUM_THR);
  printf("Inicial → Plantas: %d | Herbívoros: %d | Carnívoros: %d\n", n_plants,
         n_herb, n_carn);

  // Reservar estado, correr todos los ticks, liberar estado
  ecosystem_init(ROWS, COLS, NUM_THR, n_plants, n_herb, n_carn);
  ecosystem_run(ticks, live);
  ecosystem_free();

  return 0;
}
