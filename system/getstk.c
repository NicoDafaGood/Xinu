/* getstk.c - getstk */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getstk  -  Allocate stack memory, returning highest word address
 *------------------------------------------------------------------------
 */
char  	*getstk(
	  uint32	nbytes		/* Size of memory requested	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/

	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = (uint32) roundpage(nbytes);	/* Use mblock multiples	*/
	for(int i =1015;i>2;i--)
	{
		PDirEntry_t* page_dir = get_dir_address();
		if(!page_dir[i].present)
		{
			page_dir[i].address = get_page()>>12;
			page_dir[i].present = 1;
			init_table(get_table_address(i));
		}
		for(int j =1023;j>=0;)
		{
			int k = j-1;
			PTableEntry_t* now = get_table_address(i);
			if(now[j].present)
			{
				j = k;
				continue;
			}
			int flag  = 0;
			while((j-k)<nbytes/4096)
			{
				if(k<0)
				{
					flag = 1;
					break;
				}
				now = get_table_address(i);
				if(now[k].present)
				{
					flag = 1;
					break;
				}
				k--;
			}
			
			if(!flag)
			{
				for(int x = k + 1 ; x <= j; x++)
				{
					now = get_table_address(i);
					now[x].address = get_page() >> 12; 
					now[x].present = 1;
				}
				restore(mask);
				return MAKEVAD(i,j,(1<<12)-4);
			}
			j = k;
		}
	}

	panic("no memory");
	restore(mask);
	return NULL;
}
