// MMU
typedef Process;
typedef Iterator;
Process current_process;


// Se invoca para recalcular el working set
void computeWS(Process *p) {
    int *ptab= p->pageTable;
    for (int i= p->firstPage; i<p->lastPage; i++) {
        if (bitV(ptab[i])) { // ¿Es válida?
            if (bitR(ptab[i])) { // ¿Fue referenciada?
                setBitWS(&ptab[i], 1); // Sí, se coloca en el working set
                setBitR(&ptab[i], 0);
            } else {
                setBitWS(&ptab[i], 0); // No, se saca del working set
            } 
        } 
    } 
    }
// Se invoca cuando ocurre un pagefault,
// es decir bit V==0 o el acceso fue una escritura y bit W==0
void pagefault(int page) {
    Process *p= current_process; // propietario de la página
    int *ptab= p->pageTable;
    if (bitS(ptab[page])) // ¿Está la página en disco?
        pageIn(p, page, findRealPage()); // sí, leerla de disco
    else
        segfault(page); // no
}
// Graba en disco la página page del proceso q
int pageOut(Process *q, int page) {
    int *qtab= q->pageTable;
    int realPage= getRealPage(qtab[page]);
    savePage(q, page); // retoma otro proceso
    setBitV(&qtab[page], 0);
    setBitS(&qtab[page], 1);
    return realPage; // Retorna la página real en donde se ubicaba
}
// Recupera de disco la página page del proceso q colocándola en realPage
void pageIn(Process *p, int page, int realPage) {
    int *ptab= p->pageTable;
    setRealPage(&ptab[page], realPage);
    setBitV(&ptab[page], 1);
    loadPage(p, page); // retoma otro proceso
    setBitS(&ptab[page], 0);
    purgeTlb(); // invalida la TLB
    purgeL1(); // invalida cache L1
}
Iterator *it; // = processIterator();
