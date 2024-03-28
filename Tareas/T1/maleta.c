#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#include "maleta.h"

// Estructura para los argumentos de la función
typedef struct {
    double *w;
    double *v;
    int *z;
    int n;
    double maxW;
    int k;
} Args;

// Declaración de funcion para phtread_create
void *thread(void *args) {
    Args *pargs = (Args *)args;
    llenarMaletaSec(pargs->w, pargs->v, pargs->z, pargs->n, pargs->maxW, (pargs->k) / 8);
    return NULL;
}

// Función que calcula en paralelo en 8 threads
double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
    pthread_t pids[8];
    Args args_array[8];
    double bestSum = -1;

    for (int i = 0; i < 8; i++) {

        // Arreglo z para cada thread
        int z_thread[] = malloc(n * sizeof(int));

        // Alocando memoria para w y v
        args_array[i].w = malloc(n * sizeof(double));
        args_array[i].v = malloc(n * sizeof(double));

        // Asignación de elementos de w y v al arreglo args_array (Esto ocurre en cada thread independientemente)
        for (int j = 0; j < n; j++) {
            args_array[i].w[j] = w[j];
            args_array[i].v[j] = v[j];
            args_array[i].z[j] = z_thread[j];
        }

        // Asignación de n, maxW y k
        args_array[i].n = n;
        args_array[i].maxW = maxW;
        args_array[i].k = k;
        
        pthread_create(&pids[i], NULL, thread, &args_array[i]);

        // Después de hacer el calculo asigna el mejor array z_thread a z
        for (int i = 0; i < n; i++) {
            int sum = 0;
            if (z_thread[i] != 0) {
                sum += v[i];
            }
            if (sum > bestSum) {
                bestSum = sum;
                z = z_thread;
            }
        }
        bestSum = -1;
        free(z_thread);
    }

    // Entierro de los threads
    for (int i = 0; i < 8; i++) {
        pthread_join(pids[i], NULL);
    }

    // Free memory allocated for w and v in args_array
    for (int i = 0; i < 8; i++) {
        free(args_array[i].w);
        free(args_array[i].v);
    }

    return 0;
}