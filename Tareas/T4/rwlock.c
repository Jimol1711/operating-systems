#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include "rwlock.h"

struct rwlock {
  int num_readers;
  int writing;
  NthQueue *writers_queue;
  NthQueue *readers_queue;
};

struct request {
  int nth_ready;
  nCond cond;
} Request;

nRWLock *nMakeRWLock() {
  // Creación del rwl
  rwl->writers_queue = nth_makeQueue();
  rwl->readers_queue = nth_makeQueue();
  rwl->num_readers = 0;
  rwl->writing = 0;
}

void nDestroyRWLock(nRWLock *rwl) {
  nth_destroyQueue(rwl->writers_queue);
  nth_destroyQueue(rwl->readers_queue);
}

int nEnterRead(nRWLock *rwl, int timeout) {
  START_CRITICAL

  if (!rwl->writing && nth_emptyQueue(rwl->writers_queue)) {
    // Se acepta lector
    rwl->num_readers++;
  } else {
    // Lector queda pendiente
  }

  END_CRITICAL
  return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {
  START_CRITICAL

  if (nth_emptyQueue(rwl->readers_queue) || rwl->writing) {
    // Se acepta escritor
    rwl->writing = 1;  
  } else {
    // Escritor queda pendiente
  }

  END_CRITICAL
  return 1;
}

void nExitRead(nRWLock *rwl) {
  START_CRITICAL
  rwl->num_readers--;
  if (num_readers == 0) {
    if (!nth_emptyQueue(rwl->writers_queue)) {
      // Se acepta escritor que lleva más tiempo
    }
  }

  END_CRITICAL
}

void nExitWrite(nRWLock *rwl) {
  START_CRITICAL

  if (!nth_emptyQueue(rwl->readers_queue)) {
    // Se acepta a todos los lectores pendientes
  } else if (nth_emptyQueue(rwl->readers_queue) && 
            !nth_emptyQueue(rwl->writers_queue)) {
    // Se acepta escritor que lleva más tiempo esperando
  }

  END_CRITICAL
}
