/* freemem.c - freemem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  freemem  -  Free a memory block, returning the block to the free list
 *------------------------------------------------------------------------
 */
syscall	freemem(
	  char		*blkaddr,	/* Pointer to memory block	*/
	  uint32	nbytes		/* Size of block in bytes	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/

	mask = disable();

	nbytes = roundpage(nbytes);
	for(char *now_ptr = blkaddr;now_ptr < blkaddr+nbytes; now_ptr += 4096)
	{
		add_page(VM_to_PM(now_ptr));
		// PTableEntry_t* now = get_table_address(GETPDE(now_ptr));
		// now[GETPTE(now_ptr)].present = 0;
		// int flag = 1;
		// for(int j = 0;j<1024;j++)
		// {
		// 	if(now[j].present==1)
		// 	{
		// 		flag = 0;
		// 		break;
		// 	}
		// }
		// if(flag)
		// {
		// 	now = get_dir_address();
		// 	now[GETPDE(now_ptr)].present = 0;
		// }
	}

	restore(mask);
	return OK;
}
