#include "pss.h"       // aquí está definido el tipo Queue
#include <semaphore.h> // aquí está sem_t, sem_init, etc...

/*
 * - Utilice 2 colas FIFO globales, una para cada equipo.
 * - Cuando un hincha deba esperar para entrar al baño, cree un semáforo con 0 tickets y
 *   póngalo en la cola correspondiente. Luego, suspenda el thread solicitando un ticket
 *   a este semáforo.
 * - Cuando salga el último hincha de un equipo y haya hinchas del otro en espera,
 *   extraiga todos los semáforos de esa cola y deposite en cada uno de ellos un ticket
 *   para permitirle a los hinchas en espera entrar al baño.
 * - Utilice un semáforo para garantizar exclusión mutua en el acceso a las variables
 *   globales.
 */

enum
{
    ROJO = 0,
    AZUL = 1
};

sem_t mutex; // sem_init(&mutex, 0, 1);

Queue *queues[2]; // = {makeQueue(), makeQueue()};
int adentro[2] = {0, 0};

void entrar(int color)
{
    // el oponente del equipo AZUL es el equipo ROJO y viceversa.
    int oponente = !color;

    sem_wait(&mutex); // Entrar en zona crítica

    // Si hay hinchas del otro equipo en el baño o en la cola, se debe esperar.
    if (adentro[oponente] > 0 || !emptyQueue(queues[oponente]))
    {
        // Para esperar debemos crear un semáforo local con 0 tickets y ponerlo a la
        // cola de espera, luego debemos hace sem_wait sobre este para esperar.
        // De esta manera, el último oponente en salir del baño revisará la cola y des-
        // pertará a cada thread individualmente.

        sem_t wait;                // Crear semáforo
        sem_init(&wait, 0, 0);     // Inicializarlo con 0 tickets
        put(queues[color], &wait); // se pone el semaforo en la cola

        sem_post(&mutex); // Debemos soltar el mutex de la zona crítica explicitamente para esperar
        sem_wait(&wait);  // se pone el thread en espera con el semáforo creado

        sem_destroy(&wait); // destruir semaforo creado!

        // Notar que desde este punto no trabajaremos con variables compartidas, por lo
        // que noe stamos en una zona crítica, no es necesario tomar el mutex nuevamente
    }
    else
    {
        // Tenemos permiso para entrar inmediatamente
        adentro[color]++;
        sem_post(&mutex); // Salir zona crítica
    }
}

void salir(int color)
{
    int oponente = !color;

    sem_wait(&mutex); // Entrar a la zona crítica

    adentro[color]--; // Salir del baño

    // Si no queda nadie
    if (adentro[color] == 0)
    {
        // Si no queda nadie dentro del baño debemos despertar a todos los oponentes en
        // espera, sacarlos de la cola uno por uno y depositar un ticket en su semáforo
        while (!emptyQueue(queues[oponente]))
        {
            sem_t *wait = get(queues[oponente]); // sacamos un oponente de la cola
            sem_post(&wait);
            adentro[oponente]++; // Ingresamos el oponente de inmediato, caso contrario puede ocurrir un datarace.
            // Nota: Podríamos estar tentados a destruir el semaforo en este punto, eso
            // sería un error porque el otro thread no necesariamente ha despertado.
        }
    }

    sem_post(&mutex); // Salir zona crítica
}
