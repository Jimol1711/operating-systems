#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#include "maleta.h"

// Estructura para los argumentos de la función y el retorno en best
typedef struct {
    double *w;
    double *v;
    int *z;
    int n;
    double maxW;
    int k;
    double best;
} Args;

// Declaración de funcion para phtread_create
void *thread(void *args) {
    Args *pargs = (Args *)args;
    double *w = pargs->w;
    double *v = pargs->v;
    int *z = pargs->z;
    int n = pargs->n;
    int maxW = pargs->maxW;
    int k = pargs->k;
    double best = llenarMaletaSec(w, v, z, n, maxW, k / 8);
    pargs->best = best;
    return NULL;
}

// Función que calcula en paralelo en 8 threads
double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
    pthread_t pids[8];
    Args args_array[8];
    double bestSum = -1;

    for (int i = 0; i < 8; i++) {
        // Alocando memoria para w, v y z
        args_array[i].w = malloc(n * sizeof(double));
        args_array[i].v = malloc(n * sizeof(double));
        args_array[i].z = malloc(n * sizeof(int));

        // Asignación de elementos de w y v al arreglo args_array
        for (int j = 0; j < n; j++) {
            args_array[i].w[j] = w[j];
            args_array[i].v[j] = v[j];
        }

        // Asignación de n, maxW y k
        args_array[i].n = n;
        args_array[i].maxW = maxW;
        args_array[i].k = k;
        
        // Creación del thread
        pthread_create(&pids[i], NULL, thread, &args_array[i]);
    }

    for (int i = 0; i < 8; i++) {
        // Entierro del thread
        pthread_join(pids[i], NULL);
        // Después de hacer el calculo encuentra la mejor suma y asigna el arreglo z de ese thread a z
        if (args_array[i].best >= bestSum) {
            bestSum = args_array[i].best;
            for (int j = 0; j < n; j++) {
                z[j] = 0;
                z[j] = args_array[i].z[j];
            }
        }
    }

    // Liberar memoria de los malloc
    for (int i = 0; i < 8; i++) {
        free(args_array[i].w);
        free(args_array[i].v);
        free(args_array[i].z);
    }
    
    return bestSum;
}