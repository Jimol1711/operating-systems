#include <pthread.h>
#include <pss.h>
#include <time.h> // estructura timespec


/* Se tiene una impresora utilizada con threads
 * 1 thread la puede usar a la vez
 *
 * La impresora se debe poner modo bajo consumo 
 * si han pasado 5 mins de inactividad
 */

/*
struct timespec { //definida en time.h
    time_t tv_sec;
    long tv_nsec;
};
*/

// se tiene:
void modoBajoConsumo();
void modoUsoNormal();


// debemos implementar:
void obtenerImpresora();
void devolverImpresora();
void inicializarImpresora();

#define TRUE  1
#define FALSE 0

/* ============== NOTAS AUXILIAR ===========

    1.- Establecer uso del patron request pues debemos entregar
        la impresora por orden de llegada. 
         - Crear cola, indicar propiedad de orden FIFO
         - Armar tipo Request minimal
         - hacer mutex m y cond obtener
         - flag ocupada

    2.- Indicar que la impresora existe como un proceso aparte
        y existe un thread dedicado a dirigir el servicio de este proceso
        este thread se encarga de manejar el acceso a la impresora. 
        Los clientes tienen el deber de indicar su llegada y 
        marcar su salida

    3.- Crear funcion para obtener impresora
         - mutex, puntos gratis control
         - armar request
         - indicar llegada (signal obtener)
        Indicar que si bien existe el caso de un cambio de contexto
        inutil la impresora no es un proceso intenso por lo que 
        no genera un sobrecosto considerable

    4.- Crear funcion para devolver impresora
        - mutex
        - Desmarcar flag de ocupancia
        advertir que ahora hay que devolver la impresora y que
        se podría utilizar un nuevo signal para esto
        - signal devolver

    5.- Crear función de server
        - while true: este es un proceso sin fin
            + loops infinitos se pueden hacer con:
                - while(1), while(;), for(;;)
        - mutex
        - while ocupado, server descanza
        - si no esta ocupada hay que revisar si hay solicitudes (!emptyQueue)
            - si hay solicitudes, sacar request, marcar ocupancia, signal
        Indicar función base está lista, pero debemos implementar timeouts

    6.- Explicar timeouts, son un timer que puede afectar despertar codigo
        Como un signal automatizado, un despertador
        Para esto existe: pthread_con_timed_wait(&cond, &mutex, &timespec)

    7.- Implementar tiemout, 
        - cambiar condicion inicial (hacer ese caso else)
        - consultar hora actual (en segs) con clock_gettime(CLOCK_REALTIME, &ts)
        - sumar 5 minutos a ts.tv_sec
        - dejar en timed_wait
        - si ocurre timeout y la fila esta vacia, entrar a modo bajo consumo y esperar
            - luego uso normal


*/


// Request a usar
typedef struct {
    int ready;
    pthread_cond_t w;
} Request;

// cola de solicitudes
Queue *q;
// mutex para seccion crit
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t obtener = PTHREAD_COND_INITIALIZER;
pthread_cond_t devolver = PTHREAD_COND_INITIALIZER;
// bool para ocupancia de impresora
int ocupada;
pthread_t thread_impresora;

// iniciar cola, impresora sin uso y correr impresora
void inicializarImpresora(){
    q = makeQueue();  // cola FIFO
    ocupada = FALSE;
    pthread_create(&thread_impresora, NULL, ImpresoraServer, NULL);
}

void obtenerImpresora(){
    pthread_mutex_lock(&m);

    Request req = {FALSE, PTHREAD_COND_INITIALIZER};
    put(q, &req); // guardar request req en cola q
    pthread_cond_signal(&obtener);
    // ocupada = TRUE // no esta aca por seguridad, el server se encarga de esto 
    while (!req.ready){
        pthread_cond_wait(&req.w, &m);
    }

    pthread_mutex_unlock(&m);
}

void devolverImpresora(){
    pthread_mutex_lock(&m);

    ocupada = FALSE;
    pthread_cond_signal(&devolver);
    pthread_mutex_unlock(&m);
}

void ImpresoraServer(){
    while(TRUE){
        pthread_mutex_lock(&m);

        // si esta ocupada no se puede devolver
        while(ocupada)
            pthread_cond_wait(&devolver, &m);

        // revisar si hay threads esperando usar la impresora
        if(!emptyQueue(q)){
            // Tomar solicitud en la cola, decirle que esta listo
            Request *req = get(q);
            req -> ready = TRUE;
            ocupada = TRUE;
            pthread_cond_signal(&req->w); // notificar al thread en espera
        }
        pthread_mutex_unlock(&m);
    }
}

void ImpresoraServer(){
    while(TRUE){
        pthread_mutex_lock(&m);        

        // revisar si hay threads esperando usar la impresora
        if(emptyQueue(q)){
            // si la cola esta vacia (nadie espera)
            struct  timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            ts.tv_sec += 60*5; // asignar 5 minutos de timer
            while(emptyQueue(q)
                    && pthread_cond_timed_wait(&obtener, &m, &ts) != ETIMEDOUT
                    ){
                ;//wait por 5 mins
            }
            // si han pasado 5 mins o alguien espera
            if(emptyQueue(q)){
                // nadie espera
                modoBajoConsumo(); // bajo consumo
                // mientras nadie espere, esperar
                while (emptyQueue(q)){
                    pthread_cond_wait(&obtener, &m);
                }
                // llego alguien a pedir la impresora, renaudar
                modoUsoNormal();
            }
        }
        if(!emptyQueue(q)){
            // Tomar solicitud en la cola, decirle que esta listo
            Request *req = get(q); 
            req -> ready = TRUE;
            ocupada = TRUE;
            pthread_cond_signal(&req->w); // notificar al thread en espera
        }

        // si esta ocupada no se puede devolver
        while(ocupada){
            pthread_cond_wait(&devolver, &m);
        }


        pthread_mutex_unlock(&m);
    }
}