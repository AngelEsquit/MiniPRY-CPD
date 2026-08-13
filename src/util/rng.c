/*
 * util/rng.c
 * Utilidades sin estado, usadas por todos los pasos del pipeline que
 * necesitan aleatoriedad o un orden de vecinos al azar: aleatoriedad
 * reentrante por hilo, y las 4 direcciones vecinas.
 */
#include "../internal.h"

// Offsets de fila/columna para arriba, abajo, izquierda, derecha
const int DR[4] = {-1, 1,  0, 0};
const int DC[4] = { 0, 0, -1, 1};

// Fisher-Yates: baraja arr[] in-place usando una semilla propia del hilo
void shuffle(int *arr, int n, unsigned int *seed) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(next_rand_u32(seed) % (unsigned int)(i + 1)); // indice al azar entre 0 e i
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp; // swap
    }
}
