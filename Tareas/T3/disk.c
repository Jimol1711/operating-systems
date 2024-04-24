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
// mutex, cola normal, cola de prioridad
pthread_mutex_t m;
PriQueue *priQ1;
int disk_busy = 0;
int current_track = 0;

// Estructura para la request
typedef struct {
    int ready;
    int my_track;
    pthread_cond_t c;
} Request;

void iniDisk(void) {
    pthread_mutex_init(&m, NULL);
    priQ1 = makePriQueue();
}

void cleanDisk(void) {
    pthread_mutex_destroy(&m);
    destroyPriQueue(priQ1);
}

void requestDisk(int track) {
    pthread_mutex_lock(&m);

    // if (disk_busy || !emptyPriQueue(priQ1)) {
    Request req = {0, track, PTHREAD_COND_INITIALIZER};
    priPut(priQ1, &req, req.my_track);

    while (disk_busy)
        pthread_cond_wait(&(req.c), &m);


    // } else {
    //    disk_busy = 1;
    //    current_track = track;
    // }

    pthread_mutex_unlock(&m);
}

void releaseDisk() {
    pthread_mutex_lock(&m);

    if (!emptyPriQueue(priQ1)) {
        Request *req = (Request *)priGet(priQ1);
        current_track = req->my_track;
        // req->ready = 1;
        pthread_cond_signal(&(req->c));

        // Código de prueba
        #if 0
        Request *next = priPeek(priQ1);
        if (next->my_track >= current_track) {
            Request *req = priGet(priQ1);
            req->ready = 1;
            current_track = req->my_track;
            pthread_cond_signal(&(req->c));
        } else {
            Request *req1 = priGet(priQ1);
            priPut(priQ2, req1, req1->my_track);

        }
        #endif

    } else {
        disk_busy = 0;
    }

    pthread_mutex_unlock(&m);
}
