/* ecosystem.h - Interfaz para la simulación de ecosistema */
#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int type;
    int energy;
    int starve;
    int age;
} Cell;

void ecosystem_init(int rows, int cols, int num_threads,
                    int n_plants, int n_herb, int n_carn);
void ecosystem_run(int ticks);
void ecosystem_free(void);

#ifdef __cplusplus
}
#endif

#endif /* ECOSYSTEM_H */
