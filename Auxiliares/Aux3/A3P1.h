#include <pthread.h>


typedef unsigned Ciudad;
typedef unsigned Contenedor;
typedef struct {
  Contenedor* cont;
  unsigned id;
} Camion;

#define P 8      // Número de camiones
#define R 100    // Número max de encargos
#define TRUE 1
#define FALSE 0

void transportar(Contenedor *cont, Ciudad *orig, Ciudad *dest);
void conducir(Camion *c, Ciudad *orig, Ciudad *dest);
void cargar(Camion *c, Contenedor *cont);
void descargar(Camion *c, Contenedor *cont);
