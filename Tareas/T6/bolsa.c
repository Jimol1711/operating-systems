#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
typedef enum {EN_ESPERA, ADJUDICADO, RECHAZADO} Estado;

int mutex = OPEN;
int precio_min = 0;
char *vendedor_min;
char *comprador_ptr;
int *vendedor_spinlock_ptr;
Estado *VL_ptr;

int vendo(int precio, char *vendedor, char *comprador) {
    Estado VL = EN_ESPERA;
    int vendedor_spinlock = CLOSED;

    spinLock(&mutex);

    if (precio_min == 0 || precio < precio_min) {
        if (precio_min != 0) {
            // Hay un vendedor esperando, actualizar su estado a RECHAZADO
            *VL_ptr = RECHAZADO;
            // Desbloquear el spinlock del vendedor anterior
            *vendedor_spinlock_ptr = OPEN;
        }

        // Actualizar variables globales con la información del nuevo vendedor
        precio_min = precio;
        vendedor_min = vendedor;
        vendedor_spinlock_ptr = &vendedor_spinlock;
        comprador_ptr = comprador;
        VL_ptr = &VL;

        spinUnlock(&mutex);

        // Esperar a que un comprador compre o un nuevo vendedor rechace la oferta
        spinLock(&vendedor_spinlock);

        if (VL == ADJUDICADO) {
            strcpy(comprador, comprador_ptr); // Copiar el nombre del comprador
            return 1;
        } else {
            return 0;
        }
    } else {
        spinUnlock(&mutex);
        return 0;
    }
}

int compro(char *comprador, char *vendedor) {
    int precio;

    spinLock(&mutex);

    if (precio_min == 0) {
        // No hay vendedores
        precio = 0;
    } else {
        // Comprar la acción del vendedor con el precio mínimo
        precio = precio_min;
        strcpy(vendedor, vendedor_min);
        strcpy(comprador_ptr, comprador);

        // Actualizar el estado del vendedor a ADJUDICADO
        *VL_ptr = ADJUDICADO;
        // Desbloquear el spinlock del vendedor
        *vendedor_spinlock_ptr = OPEN;

        spinUnlock(vendedor_spinlock_ptr);

        // Resetear las variables globales
        precio_min = 0;

    }

    spinUnlock(&mutex);
    return precio;
}
