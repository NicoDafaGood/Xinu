#include <xinu.h>

// Syscall list to dispatch in kernel space

const void *syscalls[] = {
	NULL,
	&create,		// 1
	&resume,		// 2
	&recvclr,		// 3
	&receive,		// 4
	&sleepms,		// 5
	&sleep,			// 6
	&fprintf,		// 7
	&printf,		// 8
	&fscanf,		// 9
	&read,			// 10
	&open,			// 11
	&control,		// 12
	&kill,			// 13
	&getpid,		// 14
	&disable,		// 15
	&restore,		// 16
	NULL,
};

// Syscall wrapper for doing syscall in user space

uint32 do_syscall(uint32 id, uint32 args_count, ...) {
	uint32 return_value;

	// You may need to pass these veriables to kernel side:

	uint32 *ptr_return_value = &return_value;
	// args_count;
	uint32 *args_array = &args_count + args_count;
	const void * funcaddr  = syscalls[id];

	__asm__ volatile( "movl  %0,%%esi\n"
           "movl  %1,%%ebx\n"
		   "movl  %2,%%ecx\n"
		   "movl  %3,%%edx\n"
		   "int   $0x21\n"
          :
          :"g"(funcaddr),"g"(ptr_return_value),"g"(args_count),"g"(args_array)      /* input */
          :"esi","ebx","ecx","edx");

	// Your code here ...

	return return_value;
}
