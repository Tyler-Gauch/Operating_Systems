/**********************************************
 *                                            *
 *   IPC One parent process NPROC children    *
 *                                            *
 * To compile:				      *
 *   gcc -g -o skel_a3 skel_a3.c              *
 *                                            *
 **********************************************/

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "LinkedList.h"




#define NPROC	       (16)  /* number of children processes */
#define TRUE		1
#define FALSE		0
#define MARKED 		1   /* lock reserved (lock is taken by a child) */
#define NOTMARKED 	0   /* lock available (no child process owns this lock) */
#define MAXCHILDLOCKS	4   /* max resource a child can hold before requesting
			       'release locks' from the LockManager */


#define NO_DEADLOCK 		0	/* there is no deadlock */
#define DEADLOCK_DETECTED 	1	/* Deadlock detected    */

#define MAXLOCKS		50	/* Total available resources (size of the lock table) */

/* 
 * Children send this message to request a lock.
 */
#define LOCK		100	/* Child requests to lock a resource */
#define RELEASE		200	/* Child requests to release all its resources */
struct msg_requestLock {
	int lockID;	/* this a number from 0 up to (MAXLOCKS-1) */
	int Action;	/* LOCK, RELEASE */
};




/* 
 * LockManager sends status of request to children
 */
#define GRANTED 	0
#define NOT_GRANTED	1
#define YOU_OWN_IT	2
#define PREVENT		3
struct msg_LockStatus {
	int status;
	int by_child;		/* if not granded, who owns it */
};






/* 
 * Structure the LockManager holds (this is a single lock) 
 */
struct lock {
	int marked;
	int by_child;
};


/*
 * 'lock' holds all the resources 
 */
struct lock locks[MAXLOCKS];   /* MAXLOCKS locks for the manager */
struct Node links[NPROC];	/*Array to detect deadlocks */
int deadlock = NO_DEADLOCK;	/* When deadlock occurs, exit     */ 
int pid [NPROC];               	/* Process ids                    */



/* 
 * Called at the end to cleanup
 */
void finish() {
	int i;
	for(i = 0; i < NPROC; i++) 
		kill( pid[i], 9);
	exit(0);
}




/* 
 * Code for the child processes 
 */
void child (int pid, int req, int ack) { 
	int rand_lock;		/* a random lock request        */ 
	int count = 0;		/* It is used for num of locks  */

	struct msg_requestLock MSG;	/* message from child (me) to parent */
	struct msg_LockStatus  STAT; 	/* message from parent to child (me) */

	struct timeval tt;

	(void) gettimeofday(&tt, NULL);
	srand(tt.tv_sec * 1000000 + tt.tv_usec);

	for(;;) {
		MSG.lockID  = rand() % MAXLOCKS;
		MSG.Action  = LOCK;

		printf("\tChild %d: Requesting lock %d . . .\n", pid, MSG.lockID);
		fflush(stdout);

		/*
	 	 * Both calls are blocked if there is nothing to do.
	 	 */

		write( req, (char *) &MSG,  sizeof(MSG));
		read ( ack, (char *) &STAT, sizeof(STAT));

		if( STAT.status == GRANTED ) { 	    /* Success got lock */
			count++;
			printf("\tChild %d: Got lock %d (%d).\n", pid, MSG.lockID, count);
			fflush(stdout);
		}

#ifdef TRACE
		if( STAT.status == GRANTED ) 	
			printf("\tChild %d: Got lock.\n", pid);
		else if( STAT.status == NOT_GRANTED)
			printf("\tChild %d: Child %d owns this lock.\n", pid, STAT.by_child);
		else if( STAT.status == YOU_OWN_IT)
			printf("\tChild %d: I own this lock.\n", pid);

		printf("\tChild %d: Owns %d locks now.\n", pid, count);
		fflush(stdout);
#endif

		if( STAT.status == NOT_GRANTED ) {
			printf("Child %d waiting for lock %d\n", pid, MSG.lockID);
			fflush(stdout);

			/* 
			 * I will get it shortly or the LockManager
		  	 * will NOT give it to me to prevent a deadlock.
			 */
			read ( ack, (char *) &STAT, sizeof(STAT));
			if( STAT.status == GRANTED ) {
				count++;
				printf("\tChild %d: Got lock %d (%d).\n", pid, MSG.lockID, count);
			}
			else if( STAT.status == PREVENT ) {
				printf("CHILD: %d Will try again, (preventing)\n", pid);
				fflush(stdout);
			}
			else{
				printf("CHILD: %d    FATAL ERROR %d\n", pid, STAT.status);
				fflush(stdout);
				exit(-1);
			}
		}

		if( count >= MAXCHILDLOCKS ) {
			/*
			 * Child sends request to release all its locks
			 */
			printf("\tChild %d: Requesting RELEASE locks.\n", pid);
			fflush(stdout);

			MSG.Action=RELEASE;  
			write(req,(char *) &MSG,sizeof(struct msg_requestLock));

			count = 0;

			sleep(1);
		}

	} /* for(;;) */
} /* child */



void print_locktable()
{
	FILE * place = stdout;
	int i;
	fprintf(place, "====LOCK TABLE====\n");
	for(i = 0; i < MAXLOCKS; i++)
	{
		if(locks[i].marked == MARKED)
		{
			fprintf(place, "\tLock %d owned by: %d\n", i, locks[i].by_child);
		}
	}	
	fprintf(place, "=================\n\n");
}


void print_linktable()
{
	int i = 0;
	FILE * place = stdout;
	fprintf(place, "====LINK TABLE====\n");
	for(i = 0; i < NPROC; i++)
	{
		if(links[i].resource > -1)
		{
			fprintf(place, "\tProcess %d is waiting for Process %d for resource %d\n", i, links[i].waiter, links[i].resource);
		}	
	}
	fprintf(place, "================\n\n");
}



int CheckForDeadLock() {
	int i = 0;
	int count = 0;
	//only set the ones we want to check to discoverable
	for(i = 0; i < NPROC; i++)
	{
		if(links[i].resource > -1)
		{
			links[i].status = NOTDISCOVERED;
			count++;
		}
		else
		{
			links[i].status = DISCOVERED;
		}	
	}
	//check for deadlock
	if(count <= 1)
	{
		//we only have one process waiting its impossible to have a deadlock
		return NO_DEADLOCK;
	}
	for(i = 0; i < NPROC; i++)
	{
		if(links[i].status== NOTDISCOVERED)
		{
			links[i].status = DISCOVERED;
			int has_link = TRUE;
			int child = links[i].waiter;
			while(has_link == TRUE)
			{
				if(links[child].status == NOTDISCOVERED)
				{
					links[child].status = DISCOVERED;
					child = links[child].waiter;
				}
				else
				{
					return DEADLOCK_DETECTED;
				}
			}
		}		
	}

	return NO_DEADLOCK;
}



/*******************************************************
 *                                                     *
 * LockManager():                                      *
 *               Task to determine deadlock            *
 *               Also to release and give locks        *
 *		 Terminates when LiveLock is detected  *
 *******************************************************/

int LockManager( int q, struct msg_requestLock ChildRequest, int respond[NPROC] ) {
	int i;
	struct msg_LockStatus  STAT;
	int deadlock=NO_DEADLOCK;

	if( ChildRequest.Action == RELEASE ) {
		int l = 0;
		for(l = 0; l < MAXLOCKS; l++)
		{
			int x;
			if(locks[l].marked && locks[l].by_child == q)
			{
				locks[l].marked = NOTMARKED;
				for(x = 0; x < NPROC; x++)
				{
					if(links[x].resource == l && links[x].waiter == q)
					{
						locks[l].marked = MARKED;
						locks[l].by_child = x;
						STAT.status = GRANTED;
						write(respond[x], (char*) &STAT, sizeof(STAT));
					}
				}
			}
		}
	}
	
	if( ChildRequest.Action == LOCK ) {
		int t_lock;
		t_lock = ChildRequest.lockID;
		if( locks[t_lock].marked == NOTMARKED ) {
			locks[t_lock].marked = MARKED;
			locks[t_lock].by_child = q;
			STAT.status = GRANTED;
			write(respond[q], (char*)&STAT, sizeof(STAT));
		}
		else { /* lock is not free */
			if( locks[t_lock].by_child == q ) {
				/* 	
				 * Tell child that this lock is already owned
			 	 * by this (the requestor) child
				 */
				STAT.status=YOU_OWN_IT;
				write(respond[q], (char *) &STAT, sizeof(STAT));
			}
			else { /* lock taken by another child */
				links[q].waiter = locks[t_lock].by_child;
				links[q].resource = t_lock;

				/* 
				 * Now tell the child that the Lock will 
				 * not be given (because it's owned by 
				 * someone else.
				 */
				STAT.status=NOT_GRANTED;		
				STAT.by_child = locks[t_lock].by_child;
				write(respond[q], (char *) &STAT, sizeof(STAT));

				print_locktable();
				print_linktable();	

				if( CheckForDeadLock() == DEADLOCK_DETECTED ) {

					
					printf("NOW ROLLBACK\n");
					
					links[q].resource = -1;
					links[q].waiter = -1;
					links[q].status = DISCOVERED;

					/* 
					 * OK we rolledback, now notify the 
					 * child that the lock will not be
					 * given to it in order to prevent
					 * the deadlock.
					 */
					STAT.status=PREVENT;		
					write(respond[q], (char *) &STAT, sizeof(STAT));

					int i = 0;
					int good = FALSE;
					for(i = 0; i < MAXLOCKS; i++)
					{
						if(locks[i].marked == NOTMARKED)
						{
							good = TRUE;
							break;
						}
					}
					
					if(good == FALSE)
					{
						printf("================\n====LIVELOCK====\n================\n");
						print_locktable();
						print_linktable();
						finish();
						exit(-1);
					}
				}
			}
		}
	}


	//
	// if ChildRequest.Action is neither RELEASE nor LOCK, you got a protocol error.
	//



	return(deadlock);
} 





/******************************************************************
 *
 ******************************************************************/
int main(int argc, char** argv) {
	int i;
	int listen[NPROC];
	int respond[NPROC];
	int ii;
	int iii;
	struct msg_requestLock ChildRequest;

	for(iii = 0; iii < NPROC; iii++)
	{
		links[iii] = createNode(-1,-1);
	}
	print_linktable();

	/* 
	 * Arrange for termination to call the cleanup procedure 
	 */
	signal( SIGTERM, finish );


	/*
	 * initialize, don't carre about child  
	 */
	for( ii = 0; ii < MAXLOCKS; ii++) {
		locks[ii].marked = NOTMARKED;
	}


	/* 
	 * Initialize pipes and fork the children processes 
	 */
	for( i = 0; i < NPROC; i++ ) {
		int parent_to_child[2];
		int child_to_parent[2];

		/* 
	  	 * Create the child -> parent pipe. 
		 */
    		pipe(child_to_parent);
    		listen[i] = child_to_parent[0];
    		fcntl (listen[i], F_SETFL, O_NONBLOCK);

    		/* 
		 * Create the parent -> child pipe. 
		 */
    		pipe(parent_to_child);
    		respond[i] = parent_to_child[1];
    		fcntl (respond[i], F_SETFL, O_NONBLOCK);

    		/* 
		 * Create child process. 
		 */

		if ((pid[i] = fork()) == 0) {
  			/* 
	 		 * *********** Child process code. ***********
	 		 */
			signal (SIGTERM, SIG_DFL);
			close (parent_to_child[1]);
			close (child_to_parent[0]);

			child (i, child_to_parent[1], parent_to_child[0]);

			_exit(0);
    		}

  		close (child_to_parent[1]);
  		close (parent_to_child[0]);
  	}




	/* 
	 * For this assignment there will never be a deadlock because our
	 * LockManager prevents deadlocks
	 */
	while( deadlock != DEADLOCK_DETECTED ) { 
		int q;
		for( q = 0; q < NPROC; q++) {
			unsigned char buffer[2];
			int nb = read(listen[q], (char *) &ChildRequest, 
				      sizeof(ChildRequest));
			if(nb == sizeof(ChildRequest) ) {
				deadlock = LockManager(q, ChildRequest, respond);
			}
		} 
	}

	/* 
	 * Just be nice if your LockManager detects but does not
         * prevent a deadlocks, kill all children processes.
	 */
	finish();	
} 

