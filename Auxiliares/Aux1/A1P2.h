#include <pthread.h>

typedef unsigned long long ulonglong;
typedef unsigned int uint;

uint buscarFactor(ulonglong x, uint i, uint j);
uint buscarFactorParalelo(ulonglong x, uint i, uint j, uint p);