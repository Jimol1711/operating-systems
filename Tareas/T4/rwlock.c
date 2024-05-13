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
  nRWLock *rwl = malloc(sizeof(nRWLock));
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

  END_CRITICAL
  return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {
  START_CRITICAL

  END_CRITICAL
  return 1;
}

void nExitRead(nRWLock *rwl) {
  START_CRITICAL
  
  END_CRITICAL
}

void nExitWrite(nRWLock *rwl) {
  START_CRITICAL

  END_CRITICAL
}
