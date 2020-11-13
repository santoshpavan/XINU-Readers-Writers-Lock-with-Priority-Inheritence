#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef
#define NLOCKS 50
#endif

#ifndef
#define NPROC 50
#endif

#define	LFREE	'f'
#define	LUSED	'u'
#define LREAD   'r'
#define LWRITE  'w'
/* 
LNONE means no mapping for that process and lock
LNONE => neither LREAD or LWRITE
*/
#define LNONE   'u'

/*
#define NOT_WAITING  0
#define WAITING      1
#define ACQUIRED     2
*/

struct	lentry	{
    char  lstate;    /* the state LFREE or LUSED */
	int	  lqhead;	 /* q index of head of list	*/
	int	  lqtail;    /* q index of tail of list	*/
    char  ltype;     /* LREAD or LWRITE */
    char  proc_types[NPROC]; /* LREAD or LWRITE for assigned locks */
    int   nreaders;  /* number of readers */
};
struct	lentry	locktab[];
int	nextlock;

#define	isbadlock(lock_id)	(lock_id < 0 || lock_id >= NLOCKS)

int lcreate (void);
int ldelete (int);
int lock (int, int , int);
int releaseall (int, int,...);

// clkinit.c
extern unsigned long ctr1000;

// lock.c
void assignOtherWaitingReaders(int, int);

#endif
