#include <pthread.h>
#include <stdio.h>
#include <math.h>

typedef unsigned long long ulonglong;
typedef unsigned int uint;

// busca un factor del número entero x en el rango [i, j]
uint buscarFactor(ulonglong x, uint i, uint j){
    for (uint k = i; k <= j; k++){
        if (x % k == 0)
            return k;
    }
    return 0;
}


int main(){ // gcc A1P2.c -o script -lm -pthread
    ulonglong num = 117630860783701; // 688846502588399
    uint res = buscarFactor(num, 2,  (uint)sqrt((double)num));
    if (res > 0)
        printf("Un factor de %llu es : %u\n", num, res);
    else
        printf("%llu es primo\n", num);
    return 0;
}