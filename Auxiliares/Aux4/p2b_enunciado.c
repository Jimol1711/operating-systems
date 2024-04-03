// Esta es la solución incorrecta.

#include <semaphore.h>

enum
{
    ROJO = 0,
    AZUL = 1
};

sem_t mutex;  // sem_init(&mutex, 0, 1);
sem_t sem[2]; // sem_init(&sem[ROJO], 0, 1); sem_init(&sem[AZUL], 0, 1);

int esperan[2] = {0, 0};
int adentro[2] = {0, 0};

void entrar(int color)
{

    // el oponente del equipo AZUL es el equipo ROJO y viceversa.
    int oponente = !color;

    sem_wait(&mutex);

    // Si hay hinchas del otro equipo en el baño o en la cola, se debe esperar.
    if (adentro[oponente] > 0 || esperan[oponente] > 0)
    {
        esperan[color]++;
        sem_post(&mutex);      // Se debe soltar explicitamente el mutex porque vamos a esperar
        sem_wait(&sem[color]); // se pone el thread en espera
        sem_wait(&mutex);      // Debemos esperar el mutex para entrar en zona crítica nuevamente
    }

    adentro[color]++; // entramos al baño
    sem_post(&mutex);
}

void salir(int color)
{
    int oponente = !color;

    sem_wait(&mutex);

    adentro[color]--; // salimos del baño

    if (adentro[color] == 0)
    {
        // Despertar a los oponentes poniendo tantos tickets como son
        // threads hay en espera
        for (int i = 0; i < esperan[oponente]; i++)
        {
            sem_post(&sem[oponente]);
        }
        esperan[oponente] = 0;
    }

    sem_wait(&mutex);
}