/* create.c - create, newpid */

#include <xinu.h>
extern	void * ret_k2u();
local	int newpid();

/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp, savusp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr, *usaddr;		/* Stack address		*/

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundpage(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(8192,0)) == (uint32 *)SYSERR) || 
		 ((usaddr = (uint32 *)getstk(ssize,1)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}

	

	PDirEntry_t* new_dir = (PDirEntry_t*)getmem(4096);
	PTableEntry_t* new_table = (PTableEntry_t*)getmem(4096);
	PTableEntry_t* tmp = new_table;
	init_table(new_dir);
	init_table(new_table);

	for(int i = 0;i<1024;i++)
		new_dir[i].allow_write = 1;


	new_dir[0].present = 1;
	new_dir[0].address = (uint32)VM_to_PM(new_table) >> 12;
	new_dir[0].allow_user_access = 1;

	new_dir[2].present = 1;
	new_dir[2].address = (uint32)VM_to_PM(new_dir) >> 12;
	new_dir[2].allow_user_access = 1;

	for(int i = 0;i<&end;i+=4096)
	{
		new_table[GETPTE(i)].address = ((uint32)i)>>12;
		new_table[GETPTE(i)].present = 1;
		new_table[GETPTE(i)].allow_user_access = 1;
	}
	for(int i =0;i<8192;i+=4096)
	{

		new_table[GETPTE((uint32)&end + 8191 - i)].address = VM_to_PM((uint32)saddr - i)>>12;
		new_table[GETPTE((uint32)&end + 8191 - i)].present = 1;
		new_table[GETPTE((uint32)&end + 8191 - i)].allow_user_access = 1;
	}
	uint32 sssize = 0;
	for(int i = 1015;i>2;i--)
	{
		if(!new_dir[i].present)
		{
			new_table = getmem(4096);
			memset(new_table,0,sizeof(char)*4096);
			init_table(new_table);
			new_dir[i].present = 1;
			new_dir[i].address = VM_to_PM(new_table)>>12;
			new_dir[i].allow_user_access = 1;
		}
		for(int j = 1023;j>=0;j--)
		{
			if(sssize >= ssize)
				break;
			new_table[j].present = 1;
			new_table[j].address = VM_to_PM((uint32)usaddr - sssize)>>12;
			new_table[j].allow_user_access = 1;
			sssize += 4096;
		}
		if(sssize >= ssize)
			break;
	}
	
	prcount++;
	prptr = &proctab[pid];

	uint32 * new_usaddr = ((uint32)0xFE000000 - 4);
	uint32 * new_saddr = ((uint32)&end + 8192 - 4);
	/* Initialize process table entry for new process */
	prptr->cr3 = VM_to_PM(new_dir);	
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)new_saddr;
	prptr->prustkbase = (char *) new_usaddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	prptr->childstkbase = usaddr;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;


	/*initialize user stack */
	*usaddr = STACKMAGIC;
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
		*--usaddr = *a-- , --new_usaddr;	/* onto created process's stack	*/

	*--usaddr = (long)INITRET;	--new_usaddr;
	prptr->prustkptr = new_usaddr;
	prptr->childstkptr = usaddr ;
	savusp = new_usaddr;
	
	/* Initialize kernel stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)new_saddr;
	// a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	// a += nargs -1;			/* Last argument		*/
	// for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
	// 	*--usaddr = *a--;	/* onto created process's stack	*/
	/* Push arguments */
	
	*--saddr = (long)INITRET;	--new_saddr;/* Push on return address	*/
	prptr->prkstkptr = new_saddr;	
	*--saddr = 0x33;			--new_saddr;
	*--saddr = savusp;			--new_saddr;
	*--saddr = 0x00000200;		--new_saddr;
	*--saddr = 0x23;			--new_saddr;

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--saddr = (long)funcaddr;	--new_saddr;/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = (long)ret_k2u;	--new_saddr;
	*--saddr = savsp;			--new_saddr; /* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) new_saddr;	/* Start of frame for ctxsw	*/
	*--saddr = 0x00000000;		--new_saddr;/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;				--new_saddr;/* %eax */
	*--saddr = 0;				--new_saddr;/* %ecx */
	*--saddr = 0;				--new_saddr;/* %edx */
	*--saddr = 0;				--new_saddr;/* %ebx */
	*--saddr = 0;				--new_saddr;/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;			--new_saddr;/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;				--new_saddr;/* %esi */
	*--saddr = 0;				--new_saddr;/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)new_saddr);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID 32713
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
