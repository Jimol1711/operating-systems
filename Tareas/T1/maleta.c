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


double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
    pthread_t pids[8];
    Args *args_array = malloc(8 * sizeof(Args));
    
    for (int i = 0; i < 8; i++) {
        
        args_array[i].w = malloc(n * sizeof(double));
        args_array[i].v = malloc(n * sizeof(double));
        args_array[i].z = malloc(n * sizeof(int));

        for (int j = 0; j < n; j++) {
            args_array[i].w[j] = w[j];
            args_array[i].v[j] = v[j];
            args_array[i].z[j] = z[j];
        }

        args_array[i].n = n;
        args_array[i].maxW = maxW;
        args_array[i].k = k;

        
        pthread_create(&pids[i], NULL, thread, &args_array[i]);
    }
    
    for (int i = 0; i < 8; i++) {
        pthread_join(pids[i], NULL);
    }

    for (int i = 0; i < 8; i++) {
        free(args_array[i].w);
        free(args_array[i].v);
        free(args_array[i].z);
    }
    free(args_array);

    return 0; // Change the return type if needed
}
