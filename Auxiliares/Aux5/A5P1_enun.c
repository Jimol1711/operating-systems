#include <pthread.h>
#include <pss.h>
#include <time.h> // estructura timespec


/* Se tiene una impresora utilizada con threads
 * 1 thread la puede usar a la vez
 *
 * La impresora se debe poner modo bajo consumo 
 * si han pasado 5 mins de inactividad
 */

/*
struct timespec { //definida en time.h
    time_t tv_sec;
    long tv_nsec;
};
*/

// se tiene:
void modoBajoConsumo();
void modoUsoNormal();


// debemos implementar:
void obtenerImpresora();
void devolverImpresora();
void inicializarImpresora();

#define TRUE
#define FALSE
