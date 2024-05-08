#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include "rwlock.h"

struct rwlock {
  int nth_ready;
  
};

nRWLock *nMakeRWLock() {
  
}

void nDestroyRWLock(nRWLock *rwl) {
  
}

int nEnterRead(nRWLock *rwl, int timeout) {

  return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {

  return 1;
}

void nExitRead(nRWLock *rwl) {
  
}

void nExitWrite(nRWLock *rwl) {
  
}
