#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "disk.h"
#include "pss.h"

/*****************************************************
 * Agregue aca los tipos, variables globales u otras
 * funciones que necesite
 *****************************************************/
// Inicialización del mutex
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Estructura para la request
typedef struct {
    int ready;
    pthread_cond_t w;
} Request;

void iniDisk(void) {
  
}

void cleanDisk(void) {
  
}

void requestDisk(int track) {

}

void releaseDisk() {
  
}
