#include<pthread.h>
#include <limits.h> // macro INT_MAX
#include<pss.h>

// detector de plagio
// revisa que tanto se parecen 2 tareas de alumnos

typedef Tarea;
int compTareas(Tarea *t1, Tarea *t2);

// version sequencial
void masParecidasSeq(Tarea *tareas, int n, int *pi, int *pj){
    int min=INT_MAX;
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

typedef struct {
    Tarea *tareas;
    int i, j;
    int *pi, *pj;
    int *min;
    pthread_mutex_t *pm;
} Job;