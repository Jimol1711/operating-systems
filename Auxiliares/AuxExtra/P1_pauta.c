typedef Queue;
typedef nThread;
#define START_CRITICAL
#define END_CRITICAL
#define WAIT_PALITOS
Queue *q; // Pueden usar NTHQueue o Queue
int palitos[5]; // Se inicializan automáticamente en 0

void nPedir(int id) {
    START_CRITICAL 
    if ( !palitos[id] && !palitos[(id+1)%5] && emptyQueue(q) ) 
        palitos[id]= palitos[(id+1)%5]= 1; 
    else {
        nThread thisTh= nSelf(); 
        thisTh->ptr= &id; 
        suspend(WAIT_PALITOS); 
        put(q, thisTh); 
        schedule(); 
    }
    END_CRITICAL 
}
void nDevolver(int id) {
    START_CRITICAL 
    palitos[id]= palitos[(id+1)%5]= 0; 
    while (!emptyQueue(q)) { 
        nThread w= peek(q); 
        int w_id= *(int*)(w->ptr);
        // Condición para que thread continúe en espera:        
        if (palitos[w_id] || palitos[(w_id+1)%5])
            break; // Si no puede comer, los que vienen después tampoco
        get(q);
        palitos[w_id]= palitos[(w_id+1)%5]= 1; 
        setReady(w); 
    }
    schedule();
    END_CRITICAL 
}