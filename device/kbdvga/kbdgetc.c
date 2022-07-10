#include <xinu.h>
#include <kbd.h>

devcall
kbdgetc(struct dentry *devptr)
{
	char	ch;			/* Character to return		*/

	/* Wait for a character in the buffer and extract one character	*/

	wait(kbdcb.tyisem);
	ch = *kbdcb.tyihead++;

	/* Wrap around to beginning of buffer, if needed */

	if (kbdcb.tyihead >= &kbdcb.tyibuff[TY_IBUFLEN]) {
		kbdcb.tyihead = kbdcb.tyibuff;
	}

	

	/* In cooked mode, check for the EOF character */

	return (devcall)ch;
}

