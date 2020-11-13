/* Initialize the Locks */
#include <loch.h>
#include <kernel.h>
#include <proc.h>

void linit() {
    struct lentry *lptr;
    int i = 0;
    for (; i < NLOCKS ; i++) {
		lptr = &locktab[i];
        lptr->lstate = LFREE;
		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
        //#define   MININT          0x80000000
        lptr->lprio = MININT;
        int j = 0;
        for (; j < NPROC; j++) {
            lptr->proc_types[j] = LNONE;
        }
	}
}
