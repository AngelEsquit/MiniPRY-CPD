# Simulación de Ecosistema con OpenMP

Simulación de ecosistema en una grilla 2D (plantas, herbívoros, carnívoros) paralelizada con OpenMP.

## Repositorio

- https://github.com/AngelEsquit/MiniPRY-CPD

## Requisitos

- `gcc` con soporte de OpenMP (`-fopenmp`). 
- En macOS, el `clang` del sistema no trae OpenMP: hay que instalar `libomp` (`brew install libomp`) y compilar con esas flags, o instalar gcc real (`brew install gcc`) y usar `make CC=gcc-14`.

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

## Entregables y resultados

- Informe del proyecto (PDF): `docs/Informe.pdf`
- Informe editable en LaTeX: `docs/Informe.tex`
- Archivo de resultados de simulación: `resultados.txt`
- Copia del archivo de resultados para documentación: `docs/resultados.txt`

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

Las constantes de las reglas están todas en `src/internal.h`, agrupadas y comentadas al principio del archivo. Cambiar un valor ahí y volver a correr `make` alcanza para experimentar.
