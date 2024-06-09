#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
#define EN_ESPERA 0
#define ADJUDICADO 1
#define RECHAZADO 2

int spinlock = 0;
int precio_min = 0;
char *vendedor_min = NULL;
char *comprador_ptr = NULL;
int *vendedor_status_ptr = NULL;
int *vendedor_spinlock_ptr = NULL;

int vendo(int precio, char *vendedor, char *comprador) {
    int local_status = EN_ESPERA;
    int local_spinlock = 0;

    spinLock(&spinlock);
    if (precio_min == 0 || precio < precio_min) {
        precio_min = precio;
        vendedor_min = vendedor;
        vendedor_status_ptr = &local_status;
        vendedor_spinlock_ptr = &local_spinlock;
        comprador_ptr = comprador;
        spinUnlock(&spinlock);

        // Esperar hasta que un comprador compre o haya un vendedor con un precio menor
        spinLock(&local_spinlock);
        if (local_status == ADJUDICADO) {
            return 1;
        } else {
            return 0;
        }
    } else {
        spinUnlock(&spinlock);
        return 0;
    }
}

int compro(char *comprador, char *vendedor) {
    spinLock(&spinlock);
    if (precio_min == 0) {
        spinUnlock(&spinlock);
        return 0;
    } else {
        strcpy(vendedor, vendedor_min);
        strcpy((char *)comprador_ptr, comprador);
        int precio_pagado = precio_min;

        // Actualizar el estado del vendedor
        *vendedor_status_ptr = ADJUDICADO;
        spinUnlock(vendedor_spinlock_ptr);

        // Resetear los valores globales
        precio_min = 0;
        vendedor_min = NULL;
        vendedor_status_ptr = NULL;
        vendedor_spinlock_ptr = NULL;
        comprador_ptr = NULL;

        spinUnlock(&spinlock);
        return precio_pagado;
    }
}
