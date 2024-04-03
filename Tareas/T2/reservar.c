#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reservar.h"

// Defina aca las variables globales y funciones auxiliares que necesite
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void initReservar() {
}

void cleanReservar() {
}

int reservar(int k) { // k es el número de estacionamientos requerido
    // Si hay k estacionamientos contiguos los reserva y retorna id del primer estacionamiento en la serie otorgada
    // En caso contrario, espera hasta que hayan k estacionamientos contiguos disponibles.
}

void liberar(int e, int k) { // e es el índice del primer estacionamiento donde está, k es el número de estacionamientos requerido
  // Se invoca cuando un automovilista se va
  // Libera todos los estacionamientos reservados por un automovilista con reservar
} 
