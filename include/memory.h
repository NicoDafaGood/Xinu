/* memory.h - roundmb, truncmb, freestk */

#define	PAGE_SIZE	4096

/*----------------------------------------------------------------------
 * roundmb, truncmb - Round or truncate address to memory block size
 *----------------------------------------------------------------------
 */
#define	roundmb(x)	(char *)( (7 + (uint32)(x)) & (~7) )
#define	truncmb(x)	(char *)( ((uint32)(x)) & (~7) )

/*----------------------------------------------------------------------
 *  freestk  --  Free stack memory allocated by getstk
 *----------------------------------------------------------------------
 */
#define	freestk(p,len)	freemem((char *)((uint32)(p)		\
				- ((uint32)roundpage(len))	\
				+ (uint32)sizeof(uint32)),	\
				(uint32)roundpage(len) )

#define	roundpage(x)	(char *)( (4095 + (uint32)(x)) & (~4095) )
#define	truncpage(x)	(char *)( ((uint32)(x)) & (~4095) )
#define PDESHIFT	22
#define PTESHIFT	12
#define GETPDE(x)		(((uint32)x)>>22)&(0x3ff)
#define GETPTE(x)		(((uint32)x)>>12)&(0x3ff)
#define MAKEVAD(a,b,c)	(((uint32)a<<PDESHIFT) | ((uint32)b<<PTESHIFT) | ((uint32)c))

typedef struct PDirEntry
{
    unsigned present : 1;
    unsigned allow_write : 1;
    unsigned allow_user_access : 1;
    unsigned page_write_through : 1;
    unsigned page_cache_disable : 1;
    unsigned accessed : 1;
    unsigned system_defined_0 : 1;
    unsigned page_size_zero : 1;
    unsigned system_defined_1 : 4;
    unsigned address : 20;
} PDirEntry_t;

typedef struct
{
    unsigned present : 1;
    unsigned allow_write : 1;
    unsigned allow_user_access : 1;
    unsigned page_write_through : 1;
    unsigned page_cache_disable : 1;
    unsigned accessed : 1;
    unsigned dirty : 1;
    unsigned pat_zero : 1;
    unsigned global : 1;
    unsigned system_defined : 3;
    unsigned address : 20;
} PTableEntry_t;

struct	memblk	{			/* See roundmb & truncmb	*/
	struct	memblk	*mnext;		/* Ptr to next free memory blk	*/
	uint32	mlength;		/* Size of blk (includes memblk)*/
};
extern	struct	memblk	memlist;	/* Head of free memory list	*/
extern	void	*minheap;		/* Start of heap		*/
extern	void	*maxheap;		/* Highest valid heap address	*/


/* Added by linker */

extern	int	text;			/* Start of text segment	*/
extern	int	etext;			/* End of text segment		*/
extern	int	data;			/* Start of data segment	*/
extern	int	edata;			/* End of data segment		*/
extern	int	bss;			/* Start of bss segment		*/
extern	int	ebss;			/* End of bss segment		*/
extern	int	end;			/* End of program		*/
