#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <limits.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
typedef enum {PEND, RECHAZ, ADJUD} Estado;

int mutex = OPEN;
Estado offer_state = PEND;
int lowest_price = INT_MAX;
char *lowest_seller = NULL;
char *lowest_buyer = NULL;
int *seller_spinlock = NULL;

int vendo(int precio, char *vendedor, char *comprador) {
    int sl = OPEN;
    int result = 0;

    spinLock(&mutex);
    if (precio < lowest_price) {
        lowest_price = precio;
        lowest_seller = vendedor;
        seller_spinlock = &sl;
        offer_state = PEND;
        spinUnlock(&mutex);

        // Esperar a que la oferta sea adjudicada o rechazada
        spinLock(&sl);
        if (offer_state == ADJUD) {
            strcpy(comprador, lowest_buyer);
            result = 1;
        }
    } else {
        spinUnlock(&mutex);
    }
    return result;
}

int compro(char *comprador, char *vendedor) {
    int paid_price = 0;

    spinLock(&mutex);
    if (lowest_seller != NULL) {
        paid_price = lowest_price;
        strcpy(vendedor, lowest_seller);
        lowest_buyer = comprador;
        offer_state = ADJUD;

        // Despertar al vendedor
        spinUnlock(seller_spinlock);
        
        // Resetear variables globales
        lowest_price = INT_MAX;
        lowest_seller = NULL;
        lowest_buyer = NULL;
        seller_spinlock = NULL;

        spinUnlock(&mutex);
    } else {
        spinUnlock(&mutex);
    }
    return paid_price;
}
