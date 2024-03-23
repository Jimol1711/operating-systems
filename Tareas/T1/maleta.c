// Plantilla para maleta.c

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "maleta.h"

// Defina aca las estructuras y funciones adicionales que necesite
// ...
# if 0
typedef struct{
    double *w;
    double *v;
    int *z;
    int n;
    double maxW;
    double k;
} Args;

void *thread(void *p) {
    Args *pargs = (Args *) p;
    double w[] = pargs->w;
    llenarMaletaSec( );
    return NULL;
} 
# endif

// Estructura para los argumentos de la función
typedef struct {
    double *w;
    double *v;
    int *z;
    int n;
    double maxW;
    int k;
} ThreadArgs;

// Declaración de funcion para phtread_create
void *thread(void *args) {
    ThreadArgs *pargs = (ThreadArgs *)args;
    llenarMaletaSec(pargs->w, pargs->v, pargs->z, pargs->n, pargs->maxW, pargs->k / 8);
    return NULL;
};


double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {

}
