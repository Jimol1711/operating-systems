#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include "rwlock.h"

struct rwlock {
  int num_readers;
  int writing;
  NthQueue *writers_queue;
  NthQueue *readers_queue;
};

nRWLock *nMakeRWLock() {

  START_CRITICAL

  nRWLock *rwl = (nRWLock *)nMalloc(sizeof(nRWLock));
  rwl->num_readers = 0;
  rwl->writing = 0;
  rwl->writers_queue = nth_makeQueue();
  rwl->readers_queue = nth_makeQueue();

  END_CRITICAL

  return rwl;
}

void nDestroyRWLock(nRWLock *rwl) {

  START_CRITICAL

  nth_destroyQueue(rwl->writers_queue);
  nth_destroyQueue(rwl->readers_queue);
  nFree(rwl);

  END_CRITICAL
  
}

int nEnterRead(nRWLock *rwl, int timeout) {
  START_CRITICAL
  
  if (!rwl->writing && nth_emptyQueue(rwl->writers_queue)) {
    // Se acepta lector
    rwl->num_readers++;
  } else {
    // Lector queda pendiente
    nThread reader = nSelf();
    nth_putBack(rwl->readers_queue, reader);
    suspend(WAIT_RWLOCK);
    schedule();
  }

  END_CRITICAL
  return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {
  START_CRITICAL

  if (rwl->num_readers==0 && !rwl->writing) {
    // Se acepta escritor
    rwl->writing = 1; 
  } else {
    // Escritor queda pendiente
    nThread writer = nSelf();
    nth_putBack(rwl->writers_queue, writer);
    suspend(WAIT_RWLOCK);
    schedule();
  }
  
  END_CRITICAL
  return 1;
}

void nExitRead(nRWLock *rwl) {
  START_CRITICAL

  rwl->num_readers--;
  if (rwl->num_readers == 0) {
    if (!nth_emptyQueue(rwl->writers_queue)) {
      // Se acepta escritor que lleva más tiempo
      nThread writer = nth_getFront(rwl->writers_queue);
      setReady(writer);
      rwl->writing=1;
      schedule();
    }
  }

  END_CRITICAL
}

void nExitWrite(nRWLock *rwl) {
  START_CRITICAL

  rwl->writing = 0;
  if (!nth_emptyQueue(rwl->readers_queue)) {
    // Se acepta a todos los lectores pendientes
    do {
      nThread reader = nth_getFront(rwl->readers_queue);
      setReady(reader);
      rwl->num_readers++;
    } while(!nth_emptyQueue(rwl->readers_queue));
      schedule();

  } else if (nth_emptyQueue(rwl->readers_queue) && 
            !nth_emptyQueue(rwl->writers_queue)) {
    // Se acepta escritor que lleva más tiempo esperando
    nThread writer = nth_getFront(rwl->writers_queue);
    setReady(writer);
    rwl->writing=1;
    schedule();
  }

  END_CRITICAL
}
