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

static void print_usage(const char *prog_name) {
  printf("Simulación de Ecosistema con OpenMP\n");
  printf("Uso: %s <filas> <cols> <ticks> <hilos> [--live]\n", prog_name);
  printf("Parámetros:\n");
  printf("  <filas>   Número de filas de la grilla (entero positivo)\n");
  printf("  <cols>    Número de columnas de la grilla (entero positivo)\n");
  printf("  <ticks>   Número de pasos/ticks a simular (entero positivo)\n");
  printf("  <hilos>   Número de hilos OpenMP a utilizar (entero positivo)\n");
  printf("  --live    (Opcional) Muestra animación interactiva en consola\n\n");
  printf("Ejemplos:\n");
  printf("  %s 20 20 50 8\n", prog_name);
  printf("  %s 40 40 200 4 --live\n", prog_name);
}

int main(int argc, char *argv[]) {
  if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    print_usage(argv[0]);
    return 0;
  }

  // Validar argumentos mínimos antes de procesar
  if (argc < 5) {
    fprintf(stderr, "Error: Argumentos insuficientes.\n");
    print_usage(argv[0]);
    return 1;
  }

  // Leer y validar parámetros numéricos
  int ROWS = atoi(argv[1]);
  int COLS = atoi(argv[2]);
  int ticks = atoi(argv[3]);
  int NUM_THR = atoi(argv[4]);
  int live = (argc >= 6 && strcmp(argv[5], "--live") == 0);

  if (ROWS <= 0 || COLS <= 0 || ticks <= 0 || NUM_THR <= 0) {
    fprintf(stderr, "Error: Los parámetros de filas, columnas, ticks e hilos deben ser enteros positivos mayores a cero.\n");
    return 1;
  }

  // Población inicial como porcentaje del tamaño del grid
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
