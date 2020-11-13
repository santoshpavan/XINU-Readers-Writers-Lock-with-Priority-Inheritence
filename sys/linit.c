/* Initialize the Locks */
#include <loch.h>

void linit() {
    struct lentry *lptr;
    int i = 0;
    for (; i< NLOCKS ; i++) {
		(lptr = &locktab[i])->lstate = LFREE;
		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
	}
}
