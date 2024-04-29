#include "nthread-impl.h"
#include "nthread.h"
#include <pss.h>
/* ===================== en nthread-impl.h ===================== */
/* --------------------------- en el enum de estados --------------------------- */
/*

    Los estados los pueden ver como flags que se pueden
    enviar para para enviar el mensaje de que es lo que
    se quiere hacer.

    Recordar:
        - READY: listo para ejecutar
        - WAIT_X: suspender thread, X es el recurso o evento
                  relacionado con el programa.

enum{
    READY
    (...)
    WAIT_REPLY
}

*/
/* --------------------------- en el descriptor del thread --------------------------- */

/*
struct nthread{
    (...)
    NthQueue sendQueue;
    void *msg;
}

*/

// ================== Version sin timeout ===================
int nSend(nThread th, void *msg)
{
    START_CRITICAL

    // si el thread th esta en nRecieve, se pone en ready
    if (th->status == WAIT_SEND)
    {
        setReady(th);
    }

    // me
    nThread this_th = nSelf();
    
    // ponerme en la cola de threads que quiere mandar un mensaje
    nth_putBack(th->sendQueue, this_th);
    this_th->msg = msg; // guardar mensaje

    // esperar respuesta
    suspend(WAIT_REPLY);
    schedule();

    int rc = this_th->rc;
    // retornar rc
    END_CRITICAL
    return rc;
}

void *nReceive(nThread *pth, int timeout_ms)
{

    // Se ignora el timeout por ahora (ver próxima clase)
    START_CRITICAL
    nThread this_th = nSelf();

    // Si no hay mensajes en cola
    if (nth_emptyQueue(this_th->sendQueue))
    {
        // esperar a que llegue mensaje
        suspend(WAIT_SEND);
        schedule();
    }

    // tomar 1er thread de la cola y retornar su mensaje
    nThread sender_th = nth_getFront(this_th->sendQueue);
    *pth = sender_th;
    void *msg = sender_th->msg;

    END_CRITICAL
    return msg;
}

void nReply(nThread th, int rc)
{
    START_CRITICAL

    th->rc = rc;
    setReady(th);
    scheduler();

    END_CRITICAL
}
