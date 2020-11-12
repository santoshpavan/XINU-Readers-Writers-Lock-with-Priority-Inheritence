##ifndef _LOCK_H_
#define _LOCK_H_

#ifndef
#define NLOCKS 50
#endif

#define	LFREE	'f'
#define	LUSED	'u'
#define LREAD   'r'
#define LWRITE  'w'

struct	lentry	{
	char  lstate;    /* the state LFREE or LUSED */
	int	  lockcnt;   /* count for this lock		*/
	int	  lqhead;	 /* q index of head of list	*/
	int	  lqtail;    /* q index of tail of list	*/
    char  ltype;     /* LREAD or LWRITE */        
};
struct	lentry	locks[];
int	nextlock;

#define	isbadlock(lock_id)	(lock_id < 0 || lock_id >= NLOCKS)

int lcreate (void);
int ldelete (int);
int lock (int, int , int);
int releaseall (int, int,...);

#endif
