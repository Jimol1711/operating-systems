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

/* HACER ACLARACIÓN: 
    Las variables de arriba deberían ser inicializadas en una función de 
    inicialización

*/

/* ORDEN AUX

    - Explicar intuición desde la función transportar:
        Nosotros queremos modificar esto para controlar varios camiones.
        Identificamos el código presente como lo que queremos paralelizar
        Primero vemos de donde sacamos el camión para darlo, este corresponde a un
        indice el cual debemos __buscar__. Esto debe ser sin paralelización.
        Luego vemos que una vez que se termine de usar el camión este se debe entregar
        a la siguiente petición. Es decir se tiene que __desocupar__. Esto sin paralelización

    - Definir función de busqueda:
        Esta debe considerar que debe ser el camión más cercano (usar pos_origen)
        Si no se tiene un camión disponible, deberemos dejar un "ticket" (REQUEST)

        + Resquest:
            - Lo podemos ver como un ticket que tiene una variable que nos dice si "nos han llamado"
            - Además va a contener un pthread_cond que nos va a llamar como el llamado en una fila del super
            - Adicionalmente le agregamos información necesaria el funcionamiento del algoritmo diseñado

        Entonces debemos dejar una request que contenga las variables inicializadas
        Dejaremos esta request en el primer lugar de la lista que esté disponible
        Nos vamos a mimir

    - Definir func desocupar:
        En el patron request siempre antes de dejar el mutex primero buscamos la request
        adecuada a la cual despertar. Para aquello tendremos que buscar la solicitud más cercana
        a la posición del camión
        Si no existe una request que este esperando el camión se deja desocupado
        Si obtuvimos una request debemos entregarle la información del camión que
        le vamos a asignar, decirle que todo esta listo y despertarlo.

    - Cierre:
        Podemos mencionar que el uso de listas no era precisamente necesario, podría haber servido un
        diccionario. En otros problemas se podrían usar cosas, y otras cosas. Depende del problema
        Las estructuras las tenemos en el archivo pss.h

*/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // herramienta de sincronización
int ocupados[P]; // pos i es TRUE si camión está ocupado

typedef struct {
    int ready; 
    int idx;
    Ciudad *orig;
    pthread_cond_t c;
} Request;

Request *requests[R];


int buscar(Ciudad *orig){
    int idx = -1;
    // si hay camiones desocupados, se elige el mas cercano
    for(int k=0; k<P; k++){
        if(!ocupados[k]){
            // Si es el primer camion desocupado, o esta mas cerca que el camion candidato
            // entonces este camion es mi nuevo camion candidato
            if(idx == -1 || distancia(ubicaciones[k], orig) < distancia(ubicaciones[idx], orig)){
                idx = k;
            }
        }
    }
    // no hay camiones desocupados -> esperar (crear REQUEST)
    if(idx < 0){
        Request req = {FALSE, -1, orig, PTHREAD_COND_INITIALIZER};

        for(int i=0; i<R; i++){ // guardar request
            if(requests[i] == NULL){
                requests[i] = &req;
                break;
            } // mencionar que normalmente en el curso usaremos queues que nos permiten funcionar en orden de llegada
              // aca usamos una lista puesto que nuestro criterio es "la más cercana"  
        }
        // esperar request hasta que se atienda
        while (!req.ready){ // este while es porque pueden ocurrir accidentes debido a la implementación
            pthread_cond_wait(&req.c, &m);
        }
        idx = req.idx; // This solo si se despierta el request
    }
    return idx;
}


void desocupar(int idx){
    // Si hay solicitudes pendientes, elegigimos el contenedor mas cercano para darle el camión
    int req_idx = -1;
    for(int i=0; i<R; i++){
        if(requests[i] != NULL){
            if(req_idx == -1 || 
                distancia(ubicaciones[idx], requests[i]->orig) < distancia(ubicaciones[idx], requests[req_idx]->orig)){
                req_idx = i;
            }
        }
    }
    // Si no hay solicitudes pendientes, el camión entra en ocio
    if(req_idx < 0){
        ocupados[idx] = FALSE;
    } else {
        // Despertamos al cliente para que comande el camión
        requests[req_idx]->ready = TRUE;            // todo listo mi rey
        requests[req_idx]->idx = idx;               // camión que usará
        pthread_cond_signal(&requests[req_idx]->c); // despertar al cliente
        requests[req_idx] = NULL;                   // borrar la solicitud
    }
}


void transportar(Contenedor *cont, Ciudad *orig, Ciudad *dest){
    pthread_mutex_lock(&m); 
    
    // Buscar la el camion mas cercano (segun ubicaciones) al origen
    int idx = buscar(orig); // retorna indice del camion asignado
    ocupados[idx] = TRUE;                // marcamos al camión como ocupado
    Camion *c = camiones[idx];           // obtenemos al camión
    pthread_mutex_unlock(&m);

    conducir(c, ubicaciones[idx], orig);
    cargar(c, cont);
    conducir(c, orig, dest);
    descargar(c, cont);

    pthread_mutex_lock(&m);
    ubicaciones[idx] = dest;                   // actualizar ubicación del camión
    desocupar(idx);                     // entregar camión
    pthread_mutex_unlock(&m);
    
}
