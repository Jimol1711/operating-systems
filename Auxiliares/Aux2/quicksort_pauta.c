/* Pasos para la solución:

    1. Descubrir / diseñar qué parte del algoritmo podemos paralelizar efectivamente:
        Ver ppt: paralelizaremos los llamados recursivos de quicksort_seq, de manera que uno se lanza en nuevo thread.


    2. Crear estructura Args para poder ingresar argumentos a la función a paralelizar.
        ¿Que argumentos debemos pasar a la función a paralelizar?
        Debemos pasar si o si el aeglo y el rango sobre el que trabajar.
        Además debemos pasar el números de cores disponibles para realizar esta tarea!

    3. Programar la función a paralelizar (función que lanza pthread_create).
        En este caso, la función simplemente deberá llamar a quicksort (que debemos modificar)!
*/

/*
    Dentro de quicksort:

    A. Lanzar threads con argumentos correspondientes.
        Debemos lanzar un nuevo thread con la "mitad" del rango.

    B. Esperar que el trabajo paralelo sea realizado (Quizás es necesario realizar trabajo en el thread principal).
        Hacer el llamado recursivo! el thread principal debe realizar trabajo!

    C. Enterrar los threads lanzados y recolectar los resultados.
        Invocar pthread join para enterrar el thread y esperar el resultado! En este caso el resultado es que la parte del aeglo que trabajó el thread estará ordenada.

    Nota: Debemos considerar el caso base cuando ya no queden cores, es decir, no podemos lanzar más threads. Desde ese punto en adelante
    solo se deben realizar llamados recursivos normales, podemos llamar quicksort_seq
*/

#include <pthread.h>

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int particionar(int a[], int low, int high) {
    int pivot = a[high];  
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (a[j] <= pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    return (i + 1);  
}


void quicksort_seq(int a[], int i, int j){
    if (i < j){
        int h = particionar(a, i, j);
        quicksort_seq(a, i, h - 1);
        quicksort_seq(a, h + 1, j);
    }
}

typedef struct {
    int *a; // a[]
    int i, j;
    int n;
} Args;

void *thread_function(void *p)
{
    Args *arg = (Args *)p;
    quicksort(arg->a, arg->i, arg->j, arg->n); // *(arg.a) == arg->a
    return NULL;
}

void quicksort(int a[], int i, int j, int n)
{
    if (i < j){
        if (n == 1){
            quicksort_seq(a, i, j);
        }
        // Quedan cores a repartir
        else{
            int h = particionar(a, i, j);
            pthread_t pid;
            Args args = {a, i, h - 1, n / 2};
            pthread_create(&pid, NULL, thread_function, &args);
            quicksort(a, h + 1, j, n - (n / 2));
            pthread_join(pid, NULL);
        }
    }
}
