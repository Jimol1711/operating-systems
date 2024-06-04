#include <stdio.h>

#include "pss.h"
#include "spin-locks.h"
#include "pedir.h"


// Defina aca sus variables globales

// Current occupying category
int user = -1;
// Spin Locks
int slMutex = OPEN;
int slCat[2] = {CLOSED, CLOSED};
Queue *slq[2]; // waiting spin lock queue

void iniciar() {
  slq[0] = makeQueue();
  slq[1] = makeQueue();
}

void terminar() {
  destroyQueue(slq[0]);
  destroyQueue(slq[1]);
}

void pedir(int cat) {
  spinLock(&slMutex); // LOCK mutex
  // If nobody is using resources, claim them
  if (user == -1){
    user = cat;
    spinUnlock(&slMutex);
  }  else {
    // Create spin lock to wait
    int lock = CLOSED;
    put(slq[cat], &lock);
    spinUnlock(&slMutex); // UNLOCK mutex
    spinLock(&lock); // wait (safe due to being propietary)
  }
}

void devolver() {
  spinLock(&slMutex);
  // If there are threads of the other cat, concede resource to them
  if (queueLength(slq[!user]) > 0){
    user = !user; // other cat will be user
    int* other = get(slq[user]); // Unlock spin lock by arrival order
    spinUnlock(other); 
  } else if (queueLength(slq[user]) > 0){
    // If only threads of the same cat are waiting
    int* brother = get(slq[user]); // Deliver by arrival order
    spinUnlock(brother);
  } else {
    // If nobody is waiting, it's up-for-grabs
    user = -1;
  }
  spinUnlock(&slMutex);
}
