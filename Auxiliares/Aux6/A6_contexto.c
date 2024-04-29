#include "nthread-impl.h"
#include "nthread.h"
#include <pss.h>
// ====================================  Sincronizacion con nThreads ====================================

/* Envia un mensaje al thread th
 * Puede ser una estructura de datos (direccion) u otra cosa
 * y suspende el thread ejecutador hasta que llegue la respuesta 
 * 
 * Retorna el valor recibido desde nReply
 */
int nSend(nThread th, void *msg);


/* Suspende ejecución hasta recibir mensaje, escribe el 
 * descrpitor del thread que envió el mensaje en *pth
 * 
 * Retorna el msg recibido
 * 
 * If timeout > 0 retorna despues de ese tiempo */
void *nReceive(nThread *pth, int timeout);


/* Responde a un mensaje recibido de th con 
 * el codigo de retorno rc
 * 
 * No suspende la ejecucion
 */
void nReply(nThread th, int rc);


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