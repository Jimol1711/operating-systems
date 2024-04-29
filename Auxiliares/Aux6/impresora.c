#include "nthread-impl.h"
#include "nthread.h"
#include <pss.h>

// Resolver problema de impresora del Aux 5, usando mensajes
nThread impresora;
enum Mensaje
{
    OBTENER,
    DEVOLVER
};

void obtenerImpresora()
{
    int msg = OBTENER;
    nSend(impresora, &msg); // Bloquear thread hasta recibir respuesta
}

void devolverImpresora()
{
    int msg = DEVOLVER;
    nSend(impresora, &msg);
}

// ignorar argumento
int impresoraServer(void *_ignored)
{
    Queue *q = makeQueue();
    int ocupado = FALSE;

    nThread t;
    int *msg;

    while (TRUE)
    {
        if (!ocupado)
        {
            modoBajoConsumo();
            // esperar
            msg = (int *)nRecieve(&t, -1);
            modoUsoNormal();
        }
        else
        {
            // esperar
            msg = (int *)nReceive(&t, -1);
        }
        if (*msg == OBTENER)
        {
            if (ocupado)
                put(q, t);
            else
            {
                ocupado = TRUE;
                nReply(t, 0);
            }
        }
        else if (*msg == DEVOLVER)
        {
            nReply(t, 0);
            if (EmptyFifoQueue(q))
            {
                ocupado = FALSE;
            }
            else
            {
                nThread *t2 = (nThread *)get(q);
                nReply(t2, 0);
            }
        }
    }
}
