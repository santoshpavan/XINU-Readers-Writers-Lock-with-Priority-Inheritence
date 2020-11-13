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
#define NOT_WAITING  0
#define WAITING      1
#define ACQUIRED     2
*/
struct	lentry	{
    char  lstate;    /* the state LFREE or LUSED */
	int	  lqhead;	 /* q index of head of list	*/
	int	  lqtail;    /* q index of tail of list	*/
    char  ltype;     /* LREAD or LWRITE */
    //struct	qent q[];
    char   proc_types[NPROC]; /* LREAD or LWRITE */
};
struct	lentry	locktab[];
int	nextlock;

#define	isbadlock(lock_id)	(lock_id < 0 || lock_id >= NLOCKS)

int lcreate (void);
int ldelete (int);
int lock (int, int , int);
int releaseall (int, int,...);

// from clkinit.c
extern unsigned long ctr1000;

#endif
