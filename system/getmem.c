/* getmem.c - getmem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getmem  -  Allocate heap storage, returning lowest word address
 *------------------------------------------------------------------------
 */
char  	*getmem(
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
	for(int i =3;i<1015;i--)
	{
		PDirEntry_t* page_dir = get_dir_address();
		if(!page_dir[i].present)
		{

			page_dir[i].address = get_page()>>12;
			page_dir[i].present = 1;
			init_table(get_table_address(i));
		}
		for(int j =0;j<1024;)
		{
			int k = j+1;
			PTableEntry_t* now = get_table_address(i);
			if(now[j].present)
			{
				j = k;
				continue;
			}
			int flag  = 0;
			while((k-j)<nbytes/4096)
			{
				if(k>1024)
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
				k++;
			}
			
			if(!flag)
			{
				for(int x = j; x < k; x++)
				{
					now = get_table_address(i);
					now[x].address = get_page() >> 12; 
					now[x].present = 1;
				}
				restore(mask);
				return MAKEVAD(i,j,0);
			}
			j = k;
		}
	}

	panic("no memory");
	restore(mask);
	return NULL;
}
