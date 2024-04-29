#include <pthread.h>
#include <limits.h> // INT_MAX

// detector de plagio
// revisa que tanto se parecen 2 tareas de alumnos
typedef Tarea;
typedef Buffer;
int compTareas(Tarea *t1, Tarea *t2);

// version sequencial
void masParecidasSeq(Tarea *tareas, int n, int *pi, int *pj){
    int min = INT_MAX;
    for(int i=0; i<n ; i++){
        for(int j=0; j<i; j++){
            int similitud = compTareas(tareas[i], tareas[j]);
            if(similitud < min){
                min = similitud;
                *pi=i; 
                *pj=j;
            }
        }
    }
}


// implementar version para 8 cores
// patron prodcutor/consumidor de Jobs
#define P 8
#define TRUE
#define FALSE
#define BUFFERSIZE 128
typedef struct {
    Tarea *tareas;
    int i, j;
    int *pi, *pj;
    int *min;
    pthread_mutex_t *pm;
} Job;

/*  ======================== Notas Auxiliar ===========================

    1.- Definir funcion masParecidas
        Vamos a producir trabajo para que otros threads lo resuelvan
        entonces vamos a tener que armar un mutex para controlar los 
        threads
         - mutex_t m = INITIALIZER
         - copiar pegar loop secuencial pero en lugar de comparar
           vamos a hacer un job (borrar todo lo interno)
        para almacenar los job podemos usar la estructura Buffer que se vio
        en clases 
         - hacer Buffer *buf = makeBuffer(BUFFERSIZE)
         - hacer job en loop: 
            + Job job = {tareas, i, j, &pi, &pj, &min, &m};
            + Job *p_job = malloc(sizeof(Job))
            + p_job = job;
            + putBuff(buf, p_job)

    2.- Ahora podemos comenzar a paralelizar, para aquello vamos a lanzar
        threads antes del loop recien hecho para que estos esten esperando
        a que el productor cree trabajo
        - pthread_t pid[P]
        - for loop con pthread_create
        Notar que no tenemos una funcion para la cual lanzar los threads
        por lo que debemos crearla

    3.- Crear la funcion de threads, la cual no retorna nada y toma
        un puntero opaco para recibir lo que sea
         - while TRUE pues trabaja hasta que no haya trabajo
         - el trabajo viene del buffer por lo que tenemos que recibir
           este como argumento
         - obtener del buffer bufGet(buf)
        Notar que el buffer (as defined by Mateu) es bloqueante por lo 
        que va a mantener al thread hasta que este pueda obtener algo del buffer 
         - realizar la pega de comparar
         - luego revisar la similitud, envolver esta en mutex pues si
            tuvieramos una variable global de minimo esta debería ser protegida
         - liberar el puntero del job

    4.- Revisar codigo y notar que se produce y consume pero falta condicion
        de termino, por lo que en el buffer podemos poner un NULL para cada thread
        luego hacemos join y luego destruir el buffer
        Notamos que falta el comportamiento para el NULL al recibir por el
        consumidor, crear return

*/

// consumidor
void *thread_revisar(void *ptr){
    Buffer *buf = ptr;

    while(1){
        Job *p_job = bufGet(buf);
        if(p_job = NULL){
            return NULL;
        }
        int similitud = compTareas(p_job->tareas[p_job->i], p_job->tareas[p_job->j]);

        pthread_mutex_lock(p_job->pm);
        if(similitud < (p_job->min)){
            *(p_job->min) = similitud;
            *(p_job->pi)= (p_job->i); 
            *(p_job->pj)= (p_job->j);
        }
        pthread_mutex_unlock(p_job->pm);

        free(p_job);
    }
}

// productor
void masParecidas(Tarea *tareas, int n, int *pi, int *pj){
    int min = INT_MAX;
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

    Buffer *buf = makeBuffer(BUFFERSIZE);

    pthread_t pid[P];

    for(int p=0; p< P; p++){
        pthread_create(&pid[p], NULL, thread_revisar, buf);
    }

    for(int i=0; i<n ; i++){
        for(int j=0; j<i; j++){
            Job job = {tareas, i, j, &pi, &pj, &min, &m};
            Job *p_job = malloc(sizeof(Job));
            *p_job = job;
            putBuf(buf, p_job);
        }
    }

    for(int p=0; p < P; ++p){
        put(buf, NULL);
    }
    for(int p=0; p<P; ++p){
        pthread_join(pid[p], NULL);
    }
    destroyBuffer(buf);
}