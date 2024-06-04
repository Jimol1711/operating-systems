
enum
{
    OPEN,
    CLOSED
};

int mutex = OPEN;
int *q[4];
int jugadores = 0;
char **teamNames;

char **team(char *name)
{
    spinLock(&mutex);

    if (jugadores == 0)
    {
        teamNames = (char **)malloc(sizeof(char *) * 5);
    }

    teamNames[jugadores] = name;
    char **resultado = teamNames;

    if (jugadores < 4)
    { // esperar

        int lk = CLOSED;
        q[jugadores] = &lk;
        jugadores++;
        spinUnlock(&mutex);
        spinLock(&lk); // se quedará esperando
    }
    else
    { // despertar a los demás

        for (int i = 0; i < 4; i++)
        {
            spinUnlock(q[i]);
        }
        jugadores = 0;
        spinUnlock(&mutex);
    }

    return resultado; // No debo retornar la variable global
}