#include <xinu.h>
#include <kbd.h>

devcall	kbdread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buff,			/* Buffer of characters		*/
	  int32	count 			/* Count of character to read	*/
	)
{
	struct	ttycblk	*typtr;		/* Pointer to tty control block	*/
	int32	avail;			/* Characters available in buff.*/
	int32	nread;			/* Number of characters read	*/
	int32	firstch;		/* First input character on line*/
	char	ch;			/* Next input character		*/

	if (count < 0) {
		return SYSERR;
	}

	/* Block until input arrives */

	firstch = kbdgetc(devptr);

	/* Check for End-Of-File */

	if (firstch == EOF) {
		return EOF;
	}

	/* Read up to a line */

	ch = (char) firstch;
	*buff++ = ch;
	nread = 1;
	while ( (nread < count) && (ch != TY_NEWLINE) &&
			(ch != TY_RETURN)) {
		ch = kbdgetc(devptr);
		*buff++ = ch;
		nread++;
	}
	// if(kbdcb.tyihead == kbdcb.tyitail)
	// 	kbdcb.tyihead = kbdcb.tyitail = kbdcb.tyibuff;
	return nread;
}