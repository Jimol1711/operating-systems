// Los spinLocks

#include "spin-locks.h"

void spinLock(volatile int *psl) {
  do {
    while (*psl==CLOSED)
      ;
  } while (swapInt(psl, CLOSED)==CLOSED);
}

void spinUnlock(int *psl) {
  *psl= OPEN;
}
