/* meminit.c - memory bounds and free list init */

#include <xinu.h>


/* Memory bounds */

void	*minheap;		/* Start of heap			*/
void	*maxheap;		/* Highest valid heap address		*/

/* Memory map structures */

extern uint32	bootsign;		/* Boot signature of the boot loader	*/

extern  struct	mbootinfo *bootinfo;
				/* Base address of the multiboot info	*/
				/*  provided by GRUB, initialized just	*/
				/*  to guarantee it is in the DATA	*/
				/*  segment and not the BSS		*/

/* Segment table structures */

/* Segment Descriptor */



/*------------------------------------------------------------------------
 * meminit - initialize memory bounds and the free memory list
 *------------------------------------------------------------------------
 */



void *pagestk[1<<15];
int pstkptr = 0;

void add_page(void* pptr){
	pagestk[pstkptr++] = pptr;
}

void* get_page(){	
	// memset(pagestk[--pstkptr],0,4096*sizeof(char));
	return pagestk[--pstkptr];
}




void init_table(PDirEntry_t* page_dir)
{
	
	for(int i = 0; i<1024; i++)
	{
		// kprintf("%x\n",page_dir + i);
		memset(page_dir + i,0,sizeof(PDirEntry_t));
		page_dir[i].allow_write = 1;
	}
}

void init_121(PDirEntry_t* page_dir, int l, int r)
{
	l = truncpage(l);
	r = roundpage(r);
	while(l<r)
	{
		if(!page_dir[GETPDE(l)].present)
		{
			page_dir[GETPDE(l)].present = 1;
			page_dir[GETPDE(l)].address = ((uint32)get_page())>>12;
			init_table(page_dir->address<<12);
		}

		PTableEntry_t* page_table = (PTableEntry_t*)((uint32)page_dir[GETPDE(l)].address<<12);
		// init_table(page_dir->address<<12);
		page_table[GETPTE(l)].address = (l)>>12;
		page_table[GETPTE(l)].present = 1;
		l+=4096;
	}
}

void* get_page_address(int i,int j){
	return (void*)(i<<22 | j<<12 | 0);
}

void* get_table_address(int i){
	return (void*)(2<<22 | i<<12 | 0);
}


void* get_dir_address(){
	return (void*)(2<<22 | 2<<12 | 0);
}

void* VM_to_PM(void *vm)
{
	int a = GETPDE(vm);
	int b = GETPTE(vm);
	PTableEntry_t* page_add = (PTableEntry_t*)get_table_address(a);

	return page_add[b].address<<12; 

}

void* vminit(void) {

	struct	memblk	*memptr;	/* Ptr to memory block		*/
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	struct	memblk	*next_memptr;	/* Ptr to next memory block	*/
	uint32	next_block_length;	/* Size of next memory block	*/

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the free list */
	memptr = &memlist;
	memptr->mnext = (struct memblk *)NULL;
	memptr->mlength = 0;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = (void*)&end + 8192;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while(mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {

			/* This is the first free block, base address is the minheap */
			next_memptr = (struct memblk *)roundpage(minheap);

			/* Subtract Xinu image from length of block */
			next_block_length = (uint32)truncpage(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap);
		} else {

			/* Handle a free memory block other than the first one */
			next_memptr = (struct memblk *)roundpage(mmap_addr->base_addr);

			/* Initialize the length of the block */
			next_block_length = (uint32)truncpage(mmap_addr->length);
		}

		/* Add then new block to the free list */
		char *temptr = (char*) next_memptr;
		int delta = 0; 
		while(delta < next_block_length)
		{
			add_page(temptr + delta);
			delta += 4096;
		}

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	PDirEntry_t* page_dir = (PDirEntry_t*)get_page();
	init_table(page_dir);
	init_121(page_dir,0,(char*)(&end) + 8192);
	page_dir[2].address = (uint32)page_dir>>12;
	page_dir[2].present = 1;
	page_dir[2].allow_user_access = 1;
	return page_dir;
}
