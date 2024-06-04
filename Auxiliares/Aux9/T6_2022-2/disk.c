#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"
#include "priqueue.h"
#include "spinlocks.h"

// Le sera de ayuda la clase sobre semáforos:
// https://www.u-cursos.cl/ingenieria/2022/2/CC4302/1/novedades/detalle?id=431689
// Le serviran la solucion del productor/consumidor resuelto con el patron
// request y la solucion de los lectores/escritores, tambien resuelto con
// el patron request.  Puede substituir los semaforos de esas soluciones
// por spin-locks, porque esos semaforos almacenan a lo mas una sola ficha.

// Declare los tipos que necesite

// Creamos un tipo request para crear y atender solicitudes de acceso al disco.
typedef struct
{
  int *pw;   // puntero a spinlock para esperar
  int track; // pista solicitada
} Request;

// Declare aca las variables globales que necesite

int mutex;                // Spinlock para exclusión mutua de la zona crítica
int curr_track;           // Pista actual. -1 significa disco desocupado.
PriQueue *curr_queue;     // Cola con solicitudes para la vuelta actual. Inv: Todas las solicitudes
                          // en esta cola cumplen con ser mayores o iguales a curr_track
PriQueue *next_lap_queue; // Cola con solicitudes para la siguiente vuelta (luego de que el cabezal
                          // del disco vuelva al centro). Inv: Las pistas solicitadas son menores
                          // que la pista actual. Podemos considerarla como una cola de reserva.

// Agregue aca las funciones requestDisk y releaseDisk
void iniDisk(void)
{
  mutex = OPEN;
  curr_track = -1;
  curr_queue = makePriQueue();
  next_lap_queue = makePriQueue();
}

void requestDisk(int track)
{
  spinLock(&mutex);

  // Si el disco está desocupado, atendemos la solicitud inmediatamente.
  if (curr_track == -1)
  {

    curr_track = track;
    spinUnlock(&mutex);
    return;
  }
  // else

  // Creamos solicitud
  int w = CLOSED;
  Request req = {&w, track};

  // Si la pista solicitada es menor que la pista actual, debemos ingresar la solicitud a la cola
  // de reserva (siguiente vuelta)
  if (track < curr_track)
  {
    priPut(next_lap_queue, &req, track); // cola, elemento, prioridad
  }
  else
  {
    priPut(curr_queue, &req, track);
  }

  spinUnlock(&mutex); // Soltar el mutex de exclusión mutua
  spinLock(&w);       // Esperar!
}

void releaseDisk()
{
  spinLock(&mutex);

  if (emptyPriQueue(curr_queue))
  {
    // La cola de solicitudes actuales está vacía, es momento de que el cabezal se mueva a la pista
    // más cercana al centro, para eso simplemente intercambiaremos las colas, la cola de reserva
    // se convierte en la cola actual.

    // Es suficiente con que hagamos swap de los punteros de la colas, no es necesario mover
    // elementos de las mismas
    PriQueue *tmp = curr_queue;
    curr_queue = next_lap_queue;
    next_lap_queue = tmp;
  }

  // Si la cola actual no está vacía, atendemos la siguiente solicitud de pista.
  if (!emptyPriQueue(curr_queue))
  {

    Request *req = priGet(curr_queue);
    curr_track = req->track;
    int *pw = req->pw;
    spinUnlock(pw); // Despertar al core que hizo la solicitud.
  }

  else
  {
    // Ambas colas vacías.
    curr_track = -1;
  }

  spinUnlock(&mutex);
}
