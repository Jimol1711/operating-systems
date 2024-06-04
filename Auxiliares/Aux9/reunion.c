#include "spin-locks.h"

int mutex = OPEN;
int en_reunion = 0;
int *lks[8];

void ingresar(){
    spinLock(&mutex);
    en_reunion++;
    spinUnlock(&mutex);
}

void abandonar(){
    spinLock(&mutex);
    en_reunion--;
    if (en_reunion > 0){
        int w = CLOSED;
        lks[coreId()] = &w;
        spinUnlock(&mutex);
        spinLock(&w);
        return;
    } else {
        for (int k = 0; k < 8; k++)
        {
            if (k != coreId() && lks[k] != NULL)
            {
                spinUnlock(lks[k]);
                lks[k] = NULL;
            }   
        }  
    }
    spinUnlock(&mutex);
}