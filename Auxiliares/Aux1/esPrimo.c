#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "A1P2.h"

#define DEFAULT_N_THREADS 8
#define DEFAULT_N_TEST 10

// Ejemplos de primos grandes (para apreciar paralelización):
// 67280421310721
// 2 ** 31 - 1 = 2147483647
// 2 ** 61 - 1 = 2305843009213693951
typedef struct { 
    double time_taken;
    uint factor;
} Res;

Res test_par(ulonglong n, int p){
    struct timespec start, end;
    uint j = (int) sqrt(n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    uint factor = buscarFactorParalelo(n, 2, j, p);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / (10e9);
    Res res = {time_taken, factor};
    return res;
}


Res test_seq(ulonglong n){
    struct timespec start, end;
    uint j = (int) sqrt(n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    uint factor = buscarFactor(n, 2, j);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / (10e9);
    Res res = {time_taken, factor};
    return res;
}


int main(int argc, char ** argv){
    
    int n_threads = DEFAULT_N_THREADS;
    int n_test = DEFAULT_N_THREADS;
    
    char mensaje_ayuda[100 + strlen(argv[0])];
    sprintf(mensaje_ayuda, "Uso: %s [--help] [--threads [numero]] [--test [numero]]\n", argv[0]);
    
    int opt;
    int option_index = 0;
    
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"threads", required_argument, 0, 't'},
        {"test", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    // Procesar las opciones
    while ((opt = getopt_long(argc, argv, "ht:s:", long_options, &option_index)) != -1) {

        switch (opt) {
            case 'h':
                printf("%s", mensaje_ayuda);
                return 0;
            case 't':
                if (optarg != NULL && optarg[0] != '\0'){
                    n_threads = atoi(optarg);
                    break;
                }
                else{
                    fprintf(stderr, "%s", mensaje_ayuda);
                    exit(EXIT_FAILURE);
                }
                
            case 's':
                if (optarg != NULL && optarg[0] != '\0'){
                    n_test = atoi(optarg);
                    break;
                }
                    
                else{
                    fprintf(stderr, "%s", mensaje_ayuda);
                    exit(EXIT_FAILURE);
                }
                    
            default:
                fprintf(stderr, "%s", mensaje_ayuda);
                exit(EXIT_FAILURE);
        }
    }

    if (n_threads <= 0){
        fprintf(stderr, "Debe proporciona un número de thread mayor a 0.\n");
        exit(EXIT_FAILURE);
    }

    if (n_test <= 0){
        fprintf(stderr, "Debe proporciona un número de tests mayor a 0.\n");
        exit(EXIT_FAILURE);
    }

    uint dot_every_x = n_test >= 10 ? n_test / 10 : 1;    
    ulonglong n;

    printf("Ingresa número: ");
    scanf("%llu", &n);
    

    // Check si podemos correr el test
    if ((int) sqrt(n) - 2 < n_threads) {
        printf("El caso donde hay más threads que el tamaño del intervalo de búsqueda no se "
               "encuentra cubierto por la función buscarFactorParalelo\n");
        exit(EXIT_FAILURE);
    }

    double avg_seq_time = 0;
    double avg_par_time = 0;

    printf("\nTests secuencial: ");
    Res res = test_seq(n);
    uint factor_seq = res.factor;

    for(int i=0; i < n_test; i++){
        res = test_seq(n); 
        avg_seq_time += (res.time_taken / n_test);

        if (i % dot_every_x == 0){
            printf(".");
            fflush(stdout);
        }        
    }
    
    Res res_par = test_par(n, n_threads);
    uint factor_par = res_par.factor;
    
    printf("\nTests paralelo: ");
    for(int i=0; i < n_test; i++){
        res_par = test_par(n, n_threads); 
        avg_par_time += (res_par.time_taken / n_test);
        
        if (i % dot_every_x == 0){
            printf(".");
            fflush(stdout);
        } 
    }


    if (factor_par != factor_seq){
        fprintf(stderr, "\nGrave Error, ambas funciones no retornan lo mismo:  %u "
                        "(buscarFactorParalelo) v/s %u (buscarFactor)  \n", factor_par, factor_seq);
        exit(EXIT_FAILURE);
    }

    if(factor_seq){
        printf("\n\nFactor encontrado: %u es factor de %llu\n", factor_seq, n);
    } else {
        printf("\n\n%llu es primo! \n", n);
    }

    printf("\n");
    printf("BuscarFactor tomó un tiempo promedio de %f s.\n", avg_seq_time);
    printf("BuscarFactorParalelo tomó un tiempo promedio de %f s utilizando %d threads.\n", avg_par_time, n_threads);

    float speedup = ( avg_seq_time / avg_par_time - 1) * 100;
    printf("Corresponde a un speed up de: %f%%\n", speedup);

}