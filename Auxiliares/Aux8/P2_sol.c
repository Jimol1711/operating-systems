int saldos[8] = {100, ..., 100};

// Matrix de spinlocks - Indica que cores quieren robarle a otro.
// Si m[i][j] != NULL: i quiere robarle a j
int *m[8][8] = {NULL, ..., NULL};

int mutex = OPEN;

void robar(int desde, int cantidad)
{

    int id = coreId(); // identificador del core actual

    spinLock(&mutex);

    while (saldos[desde] < cantidad)
    {
        int w = CLOSED;
        m[id][desde] = &w;
        spinUnlock(&mutex);
        spinLock(&w);
        spinLock(&mutex);
    }

    saldos[desde] -= cantidad;
    saldos[id] += cantidad;

    for (int i = 0; i < 8; i++)
    {

        if (i != id && m[i][id] != NULL)
        {
            int *pw = m[i][id];
            m[i][id] = NULL;
            spinUnlock(pw);
        }
    }

    spinUnlock(&mutex);
}
