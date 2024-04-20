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
Queue *q;
PriQueue *priQ;
int disk_busy = 0;

// Estructura para la request
typedef struct {
    int ready;
    int my_track;
    pthread_cond_t c;
} Request;

void iniDisk(void) {
    pthread_mutex_init(&m, NULL);
    q = makeQueue();
    priQ = makePriQueue();
}

void cleanDisk(void) {
    pthread_mutex_destroy(&m);
    destroyQueue(q);
    destroyPriQueue(priQ);
}

void requestDisk(int track) {
    pthread_mutex_lock(&m);

    if (disk_busy || !emptyPriQueue(priQ)) {
        Request *req = malloc(sizeof(Request));
        req->ready = 0;
        req->my_track = track;
        pthread_cond_init(&(req->c), NULL);
        priPut(priQ, req, req->my_track);

        while (!req->ready)
            pthread_cond_wait(&(req->c), &m);

        free(req);
    } else {
        disk_busy = 1;
    }

    pthread_mutex_unlock(&m);
}

void releaseDisk() {
    pthread_mutex_lock(&m);

    if (!emptyPriQueue(priQ)) {
        Request *req = (Request *)priGet(priQ);
        req->ready = 1;
        pthread_cond_signal(&(req->c));
    } else {
        disk_busy = 0;
    }

    pthread_mutex_unlock(&m);
}
