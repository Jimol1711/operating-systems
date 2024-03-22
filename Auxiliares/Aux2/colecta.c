#include <pthread.h>

typedef struct {
    ;
} Colecta;

/*
    Esta función crea una Colecta.
    - fija la meta.
    - inicializa el mutex.
    - inicializa la condición.

    double meta: meta de la colecta.

    Retorna un puntero a la Colecta creada.
*/
Colecta *nuevaColecta(double meta){
    ;
}

/*
    Esta función es para aportar a una Colecta:

    Colecta *colecta: puntero a la colecta a la que se aportara:
    double monto: monto que aportará este thread a la colecta.

    Retorna el monto efectivamente aportado a la colecta (si el saldo de la colecta
    es menor al monto a aportar, se retorna el saldo, si no el monto).
*/
double aportar(Colecta *colecta, double monto){
    ;
};
