/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>

unsigned long currSP; /* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched() {

	STATWORD ps;
	disable(ps);
	register struct pentry *optr; /* pointer to old process entry */
	register struct pentry *nptr; /* pointer to new process entry */
	static int epoch = 0;
	int i;

	if (getschedclass() == LINUXSCHED) {
		optr = &proctab[currpid];

		if (currpid != NULLPROC) {
			optr->pcounter -= (QUANTUM - preempt);
			optr->pgoodness -= (QUANTUM - preempt);
			epoch -= (QUANTUM - preempt);
		} else {
			//kprintf("NULL PROC %d\n",optr->pcounter);
			epoch = 0;
		}

		//If current epoch is completer
		if (epoch <= 0) {
			epoch = 0;
			q[rdyhead].qnext = rdytail;
			q[rdytail].qprev = rdyhead;
			for (i = NPROC-1; i >0; --i) {
				//proctab[i].pprio = proctab[i].pnewprio;
				if (proctab[i].pstate != PRFREE) {

					if (proctab[i].pcounter <= 0) {
						proctab[i].pquantum = proctab[i].pprio;
					} else {
						proctab[i].pquantum = (proctab[i].pcounter / 2)
								+ proctab[i].pprio;
					}

					epoch += proctab[i].pquantum;
					proctab[i].pcounter = proctab[i].pquantum;
					proctab[i].pgoodness = proctab[i].pprio
							+ proctab[i].pcounter;

					if (proctab[i].pstate == PRREADY)
						insert(i, rdyhead, proctab[i].pgoodness);
				}
			}
			if (currpid != NULLPROC)
				insert(NULLPROC, rdyhead, proctab[i].pgoodness);
		}
//			kprintf("\n");

		int next_proc;
		int prev = q[rdytail].qprev;

		if (optr->pcounter > 0 && optr->pstate == PRCURR) {
			enable(ps);

			if (proctab[prev].pcounter > optr->pcounter)
				next_proc = prev;
			else {
				preempt = QUANTUM;
				return (OK);
			}
		} else {

			while (prev != rdyhead) {
				if (proctab[prev].pcounter > 0) {
					next_proc = prev;
					break;
				}
				prev = q[prev].qprev;
			}

			if (prev == rdyhead)
				next_proc = NULLPROC;
		}

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pgoodness);
		}
		/* remove highest priority process at end of ready list */

		nptr = &proctab[currpid = dequeue(next_proc)];
		nptr->pstate = PRCURR; /* mark it currently running	*/

#ifdef	RTCLOCK
		preempt = QUANTUM; /* reset preemption counter	*/
#endif

		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp,
				(int) nptr->pirmask);

		/* The OLD process returns here when resumed. */
		enable(ps);
		return OK;
	} else if (getschedclass() == DEFAULTSCHED) {
		if (((optr = &proctab[currpid])->pstate == PRCURR)
				&& (lastkey(rdytail) < optr->pprio)) {
			enable(ps);
			return (OK);
		}

		/* force context switch */

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[(currpid = getlast(rdytail))];
		nptr->pstate = PRCURR; /* mark it currently running	*/
#ifdef	RTCLOCK
		preempt = QUANTUM; /* reset preemption counter	*/
#endif

		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp,
				(int) nptr->pirmask);
		/* The OLD process returns here when resumed. */
		enable(ps);
		return OK;
	}

}
