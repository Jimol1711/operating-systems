#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "nthread.h"

#include "disk.h"
#include "pss.h"

// mutex, cola normal, cola de prioridad
pthread_mutex_t m;
PriQueue *priQ;
PriQueue *priQ2;
Queue *q;
int disk_busy;
int current_track;

// Estructura para la request
typedef struct {
    int ready;
    int my_track;
    pthread_cond_t c;
} Request;

void iniDisk(void) {
    pthread_mutex_init(&m, NULL);
    priQ = makePriQueue();
    priQ2 = makePriQueue();
    q = makeQueue();
    disk_busy = 0;
    current_track = 0;
}

void cleanDisk(void) {
    pthread_mutex_destroy(&m);
    destroyPriQueue(priQ);
    destroyPriQueue(priQ2);
    destroyQueue(q);
}

void requestDisk(int track) {
    pthread_mutex_lock(&m);

    if(!disk_busy) {
        disk_busy = 1;
        current_track = track;
    } else {
        Request req = {0, track, PTHREAD_COND_INITIALIZER};
        if (track >= current_track) {
            priPut(priQ, &req, track);
        } else {
            priPut(priQ2, &req, track);
        }
        while (!req.ready)
            pthread_cond_wait(&(req.c), &m);
    }

    pthread_mutex_unlock(&m);
}

void releaseDisk() {
    pthread_mutex_lock(&m);

    if (!emptyPriQueue(priQ)) {
        current_track = priBest(priQ);
        Request *req = priGet(priQ);
        req->ready = 1;
        pthread_cond_signal(&(req->c));
    } else {
        while(!emptyPriQueue(priQ2)) {
            int my_track = priBest(priQ2);
            Request *lower = priGet(priQ2);
            priPut(priQ, lower, my_track);
        }
        if (emptyPriQueue(priQ)) {
            disk_busy = 0;
        } else {
            current_track = priBest(priQ);
            Request *req = priGet(priQ);
            req->ready = 1;
            pthread_cond_signal(&(req->c));
        }
    }

    pthread_mutex_unlock(&m);
}
