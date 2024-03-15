#include <pthread.h>
#include <stdio.h>
#include <math.h>
#define P 2

typedef unsigned long long ulonglong;
typedef unsigned int uint;

// busca un factor del número entero x en el rango [i, j]
uint buscarFactor(ulonglong x, uint i, uint j){
    for (uint k = i; k <= j; k++){
        if (x % k == 0)
            return k;
    }
    return 0;
}

typedef struct{
    ulonglong x;
    uint i;
    uint j;
    uint res;
} Args;

void *thread(void *p){
    Args *args = (Args *)p;
    ulonglong x = args->x;
    uint i = args->i;
    uint j = args->j;

    args->res = buscarFactor(x, i, j);
    return NULL;
}

// busca un factor del número entero x en el rango [i, j]
// utilizando P cores
uint buscarFactorParalelo(ulonglong x, uint i, uint j){
    pthread_t pid[P];
    Args args[P];

    int intervalo = (j - i) / P;
    for (int k = 0; k < P; k++){
        args[k].x = x;
        args[k].i = i + intervalo * k;
        args[k].j = i + intervalo * (k + 1) - 1;

        pthread_create(&pid[k], NULL, thread, &args[k]);
    }

    uint factor = 0;
    for (int k = 0; k < P; k++){
        pthread_join(pid[k], NULL);
        if (args[k].res != 0)
            factor = args[k].res;
    }
    return factor;
}


