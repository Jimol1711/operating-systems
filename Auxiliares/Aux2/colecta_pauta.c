#include <pthread.h>

/*
    Colecta corresponde a una estructura que será compartida entre varios threads.
    Cada thread quiere aportar a una colecta, pero si dos o más threads lo hacen al mismo tiempo se
    pueden provocar dataraces, por lo tanto debemos proteger el acceso a esta con herramientas de
    sincronizacion. En este caso se utilizarán mutex y condiciones.

    La estructura colecta debe tener la meta a recolectar (basta con el saldo).
    Además debemos tener un mutex y condicion en cada colecta, que protegerán el acceso a ese saldo.

    Un mutex y condición global no son útiles en este caso!
    - Un mutex global bloquearía el acceso a todas las colectas diferentes al mismo tiempo.
    - Una condición global despertaría a todos los threads que están esperando diferentes colectas.
*/
typedef struct
{
    double meta;
    pthread_mutex_t m;
    pthread_cond_t c;
} Colecta;

/*
    Esta función crea una Colecta.
    - fija la meta.
    - inicializa el mutex.
    - inicializa la condición.

    double meta: meta de la colecta.

    Retorna un puntero a la Colecta creada.
*/
Colecta *nuevaColecta(double meta)
{
    Colecta *colecta = (Colecta *)malloc(sizeof(Colecta));
    colecta->meta = meta;
    pthread_mutex_init(&colecta->m, NULL);
    pthread_cond_init(&colecta->c, NULL);
    return colecta;
}

/*
    Esta función es para aportar a una Colecta:

    - Esta función restará monto de la meta de la colecta.
    - Si la meta aún no ha sido alcanzada el thread se quedará en espera a que sea
    alcanzada, la espera se realiza con una condicion.
    - Si la meta es alcanzada, se deben despertar a todos los threads que están e
    esperando la condición.

    Colecta *colecta: puntero a la colecta a la que se aportara:
    double monto: monto que aportará este thread a la colecta.

    Retorna el monto efectivamente aportado a la colecta (si el saldo de la colecta
    es menor al monto a aportar, se retorna el saldo, si no el monto).

*/
double aportar(Colecta *colecta, double monto)
{
    pthread_mutex_lock(&colecta->m);

    if (monto < colecta->meta)
        colecta->meta -= monto;
    else
    {
        monto = colecta->meta;
        colecta->meta = 0;
        pthread_cond_broadcast(&colecta->c);
    }

    while (colecta->meta > 0)
        pthread_cond_wait(&colecta->c, &colecta->m);

    pthread_mutex_unlock(&colecta->m);
    return monto;
};
