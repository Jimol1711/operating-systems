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
PriQueue *priQ;
Queue *q;
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
    priQ = makePriQueue();
}

void cleanDisk(void) {
    pthread_mutex_destroy(&m);
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
        current_track = track;
    }

    pthread_mutex_unlock(&m);
}

void releaseDisk() {
    pthread_mutex_lock(&m);

    #if 1
    if (!emptyPriQueue(priQ)) {
        Request *req = priGet(priQ); // Este llega hasta el cuarto test
        if (req->my_track >= current_track) {
            req->ready = 1;
            current_track = req->my_track;
            pthread_cond_signal(&(req->c));
        } else {
            put(q, req);
        }
    } else {
        disk_busy = 0;
    }
    #endif

    #if 0
    if (!emptyPriQueue(priQ)) {
        Request *next;
        do {
            next = priPeek(priQ);
            if (next->my_track < current_track) {
                Request *lower = priGet(priQ);
                put(q,lower);
            } else {
                while(!emptyQueue(q)) {
                    Request *putBack = get(q);
                    priPut(priQ,putBack,putBack->my_track);
                }
            }
        } while(next->my_track < current_track || !emptyQueue(q));
    } else {
        disk_busy = 0;
    }
    #endif

    #if 0

    if (!emptyPriQueue(priQ)) {
        Request *req = (Request *)priGet(priQ);
        req->ready = 1;
        pthread_cond_signal(&(req->c));
    } else {
        // Liberar el disco y avanzar al siguiente track según la estrategia C-SCAN
        disk_busy = 0;
        if (!emptyQueue(q)) {
            int next_track = current_track + 1;

            while (!emptyQueue(q)) {
                Request *req = (Request *)get(q);
                if (req->my_track == next_track) {
                    disk_busy = 1;
                    current_track = next_track;
                    pthread_cond_signal(&(req->c));
                    free(req);
                    break;
                } else {
                    put(q, req);
                }
            }
        }
    }

    #endif

    pthread_mutex_unlock(&m);
}
