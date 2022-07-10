/* xsh_uptime.c - xsh_uptime */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------
 * xsh_uptime - shell to print the time the system has been up
 *------------------------------------------------------------------------
 */
shellcmd xsh_test(int nargs, char *args[])
{
	syscall_printf("qwq\n");
	syscall_printf("qw\tq");
	syscall_printf("qw\t\tq\n");
	return 0;
}
