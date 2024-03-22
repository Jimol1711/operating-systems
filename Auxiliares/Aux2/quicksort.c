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
    ;
} Args;

void *thread_function(void *p){
    ;
}

void quicksort(int a[], int i, int j, int n){
    ;
}
