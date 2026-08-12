# Simulación de Ecosistema con OpenMP

Simulación de ecosistema en una grilla 2D (plantas, herbívoros, carnívoros) paralelizada con OpenMP. Para el diseño completo (reglas, arquitectura, mecanismo de sincronización, resultados) ver [`README.tex`](README.tex) — este archivo es solo la guía rápida de compilación y ejecución.

## Requisitos

- `gcc` con soporte de OpenMP (`-fopenmp`). En Linux esto viene por defecto con casi cualquier instalación de gcc.
- En macOS, el `clang` del sistema no trae OpenMP: hay que instalar `libomp` (`brew install libomp`) y compilar con esas flags, o instalar gcc real (`brew install gcc`) y usar `make CC=gcc-14` (o la versión que corresponda).

## Compilar

```bash
make            # genera build/*.o y el binario ./ecosystem
make clean      # borra build/ y el binario
```

El Makefile detecta automáticamente cualquier archivo `.c` nuevo bajo `src/`, no hace falta tocarlo al agregar archivos.

## Ejecutar

```bash
./ecosystem <filas> <cols> <ticks> <hilos> [--live]
```

Ejemplos:

```bash
./ecosystem 20 20 50 8              # corrida normal, reporte cada 10 ticks
./ecosystem 40 40 200 4 --live      # animación en vivo, coloreada, en la terminal
```

Sin `--live`, se imprime el conteo de población en los ticks 1, 5 y cada 10 ticks, más el mapa de caracteres (`P`/`H`/`C`) si la grilla es de 30x30 o menos. Con `--live`, la terminal se limpia y redibuja en cada tick a ~12 fps, sin límite de tamaño de grilla.

La población inicial es 30% plantas, 10% herbívoros, 10% carnívoros del total de celdas (`src/main.c`).

## Estructura del proyecto

```
include/ecosystem.h    API pública (Cell, ecosystem_init/run/free)
src/
  internal.h            Estado y helpers compartidos (privado)
  main.c                 CLI
  core/                  Estado, mapas de distancia, orquestación del tick, apply
  species/               Una decisión de movimiento por especie
  sync/                  Resolución de conflictos de destino
  io/                    Salida de consola (snapshot y --live)
  util/                  RNG
Makefile
README.tex              Informe completo del proyecto
```

## Ajustar parámetros de la simulación

Las constantes de las reglas (probabilidades de reproducción, energía, ticks de hambre, edad máxima, radio de los mapas de distancia, prioridades de conflicto) están todas en `src/internal.h`, agrupadas y comentadas al principio del archivo. Cambiar un valor ahí y volver a correr `make` alcanza para experimentar.
