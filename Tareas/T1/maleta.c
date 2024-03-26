// Plantilla para maleta.c

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
    llenarMaletaSec(pargs->w, pargs->v, pargs->z, pargs->n, pargs->maxW, pargs->k / 8);
    return NULL;
}

// Función que calcula en paralelo en 8 threads
double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
    pthread_t pids[8];
    Args args_array[8];

    for (int i = 0; i < 8; i++) {

        // Arreglo z para cada thread
        int *z_thread = malloc(n * sizeof(int));

        // Asignación de elementos de w al arreglo args_array (Esto ocurre en cada thread independientemente)
        for (int j = 0; j < n; j++) {
            args_array[i].w[j] = w[j];
            args_array[i].v[j] = v[j];
        }

        // Asignación de n, maxW y k
        args_array[i].n = maxW;
        args_array[i].maxW = maxW;
        args_array[i].k = k;
        
        pthread_create(&pids[i], NULL, thread, &args_array[i]);

        z_thread = args_array[i].z;

        free(z_thread);
    }

    // Entierro de los threads
    for (int i = 0; i < 8; i++) {
        pthread_join(pids[i], NULL);
    }

    return 0;
}
