#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
int mutex = OPEN;
int precio_min = 0;
char *vendedor_min = NULL;
char *comprador_ptr = NULL;
int *vendedor_spinlock_ptr = NULL;
Estado *VL_ptr = NULL;

typedef enum {EN_ESPERA, ADJUDICADO,RECHAZADO} Estado;

int vendo(int precio, char *vendedor, char *comprador) {
    spinLock(&mutex);
    if (precio < precio_min && vendedor_min != NULL) {
        // Espera hasta que aparezca comprador o vendedor con menor precio
        Estado VL = EN_ESPERA;
        int w = CLOSED;
        vendedor_spinlock_ptr = &w;
        spinUnlock(&mutex);
        spinLock(&w);
        VL = *VL_ptr;
        if (VL == RECHAZADO) {
            spinUnlock(&mutex);
            return 0;
        } else {
            precio_min = precio;
            vendedor_min = vendedor;
            comprador = comprador_ptr;
            spinUnlock(&mutex);
            return 1;
        }
    } else if (vendedor_min == NULL) {
        // Espera hasta que aparezca comprador
    } else {
        // Caso en que su precio es mayor, entonces falso inmediatamente
        spinUnlock(&mutex);
        return 0;
    }
    spinUnlock(&mutex);
}

int compro(char *comprador, char *vendedor) {
    spinLock(&mutex);
    if (vendedor_min == NULL) {
        spinUnlock(&mutex);
        return 0;
    } else {
        vendedor_min = vendedor;
        spinUnlock(&mutex);
        return precio_min;
    }
}
