#include "A3P1.h"

/* Con el tiempo la empresa creció y ahora tienen 8 camiones y por lo tanto
   Necesitan una nueva forma de manejar el transporte de estos para poder
   distribuir los camiones entree los encargos de carga.

   En particular se nos pide redefinir la función transporte, añadiendo funciones
   para:
      - poder buscar camiones disponibles
      - marcar que se está desocupando un camión

   La empresa nos dió lo siguiente para hacer el trabajo:
*/

#define P 8      // Número de camiones
#define R 100    // Número max de encargos
#define TRUE 1
#define FALSE 0

Camion *camiones[P];    // Los camiones
Ciudad *ubicaciones[P]; // Ciudades en las cuales se trabaja
double distancia(Ciudad * orig, Ciudad *dest); // dest entre 2 ciudades

/* Patron Request
 * 
 * Cada thread tiene su propia condicion para asi evitar 
 * despertar a todos los threads e ir llamando espeficicos 
 * threads a despertarse cuando corresponde
**/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // herramienta de sincronización
int ocupados[P]; // pos i es TRUE si camión está ocupado

typedef struct {
} Request;

Request *requests[R];


int buscar(Ciudad *orig){
}


void desocupar(int idx){
}


void transportar(Contenedor *cont, Ciudad *orig, Ciudad *dest){
}
