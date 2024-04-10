#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reservar.h"

pthread_mutex_t mutex;
pthread_cond_t cond;
int estacionamientos[10];
// Variables para manejar el orden de llegada
int ticket_dist;
int display;

void initReservar() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    ticket_dist = 0;
    display = 0;
    for (int i = 0; i < 10; i++) {
        estacionamientos[i] = 0;
    }
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

int reservar(int k) {
    pthread_mutex_lock(&mutex);
    // Reserva uno, aumenta la cola
    int my_ticket = ticket_dist++;
    // Si no es mi ticket, esperar
    while (my_ticket != display) {
        pthread_cond_wait(&cond, &mutex);
    }

    int i, j;
    while (1) {
        // Buscar k estacionamientos contiguos disponibles
        for (i = 0; i <= 10 - k; i++) {
            for (j = i; j < i + k; j++) {
                if (estacionamientos[j] == 1) 
                    break; // Estacionamiento ocupado
            }
            if (j == i + k) 
                break; // Se encontraron k estacionamientos contiguos disponibles
        }
        if (i <= 10 - k) 
            break; // Se encontraron k estacionamientos contiguos disponibles
        // Si no hay suficientes estacionamientos disponibles, esperar
        pthread_cond_wait(&cond, &mutex);
    }

    // Reservar los estacionamientos encontrados
    for (j = i; j < i + k; j++) {
        estacionamientos[j] = 1;
    }

    display++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    return i;
}

void liberar(int e, int k) {
    pthread_mutex_lock(&mutex);
    // Liberar los estacionamientos reservados
    for (int i = e; i < e + k; i++) {
        estacionamientos[i] = 0;
    }
    // Notificar a otros vehículos en espera
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
} 