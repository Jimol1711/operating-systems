#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reservar.h"

pthread_mutex_t mutex;
pthread_cond_t cond;
int disponibilidad[10] = {0}; // 0 significa disponible, 1 significa ocupado
int colaEspera = 0;

void initReservar() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

int reservar(int k) { // k es el número de estacionamientos requerido
    // Si hay k estacionamientos contiguos los reserva y retorna id del primer estacionamiento en la serie otorgada
    // En caso contrario, espera hasta que hayan k estacionamientos contiguos disponibles.
    pthread_mutex_lock(&mutex);
    int i, j;
    while (1) {
        // Buscar k estacionamientos contiguos disponibles
        for (i = 0; i <= 10 - k; ++i) {
            for (j = i; j < i + k; ++j) {
                if (disponibilidad[j] == 1) break; // Estacionamiento ocupado
            }
            if (j == i + k) break; // Se encontraron k estacionamientos contiguos disponibles
        }
        if (i <= 10 - k) break; // Se encontraron k estacionamientos contiguos disponibles
        // Si no hay suficientes estacionamientos disponibles, esperar
        colaEspera++;
        pthread_cond_wait(&cond, &mutex);
        colaEspera--;
    }

    // Reservar los estacionamientos encontrados
    for (j = i; j < i + k; ++j) {
        disponibilidad[j] = 1;
    }

    pthread_mutex_unlock(&mutex);
    return i;
}

void liberar(int e, int k) { // e es el índice del primer estacionamiento donde está, k es el número de estacionamientos requerido
  // Se invoca cuando un automovilista se va
  // Libera todos los estacionamientos reservados por un automovilista con reservar
    pthread_mutex_lock(&mutex);
    // Liberar los estacionamientos reservados
    for (int i = e; i < e + k; ++i) {
        disponibilidad[i] = 0;
    }
    // Notificar a otros vehículos en espera
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
} 
