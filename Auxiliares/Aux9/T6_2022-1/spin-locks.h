// Los spinLocks

enum { OPEN, CLOSED };

int swapInt(volatile int *psl, int val);
void spinLock(volatile int *psl);
void spinUnlock(int *psl);
