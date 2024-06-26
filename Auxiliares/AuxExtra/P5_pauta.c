// MMU
typedef Process;
typedef Iterator;
Process current_process;

// Graba en disco la página page del proceso q
int pageOut(Process *q, int page) {
    int *qtab= q->pageTable;
    int realPage= getRealPage(qtab[page]);
    if (bitD(&qtab[page]))
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
    setBitD(&ptab[page], 0);
    setBitS(&ptab[page], 0);
    purgeTlb(); // invalida la TLB
    purgeL1(); // invalida cache L1
}
Iterator *it; // = processIterator();
