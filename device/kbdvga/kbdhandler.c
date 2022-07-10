#include <xinu.h>
#include <kbd.h>

struct kbdcblk kbdcb;
int row=0,col=0;
uint16 *charbase = 0xB8000;

void showpointer()
{
	int pos = col + 80*row;
	outb(0x3D4, 14);
	outb(0x3D5, pos >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, pos);
	*((uint16*)0xB8000 + pos) = ' ' | 0x0700;
}

devcall kbdinit(){
	set_evec(0x21, (uint32)kbddisp);
	// __asm__ volatile ("int $0x21\n");
	// kprintf("nmsl\n");
	kbdcb.tyihead = kbdcb.tyitail = &kbdcb.tyibuff[0];
	kbdcb.tyisem = semcreate(0);
	kbdcb.tyicursor = 0;
	col = row = 0;
	showpointer();
	for(int i = 0;i<2000;i++)
	{
		*(charbase + i)= (0 & 0xff) | 0x0700 ;
	}
	return 0;
}


void fb_erase1(){
	col--;
	if(col<0)
		col = 79,row--;
	if(row<0)
		row = col = 0;
	int pos = col + 80*row;
	*(charbase + pos) = (0 & 0xff) | 0x0700;
}

devcall fbputc(struct dentry *devptr,char ch)
{
	
	int pos = col + 80*row;
	if(ch!='\n' && ch!='\b' && ch != '\t')
	{
		*(charbase + pos) = (ch & 0xff) | 0x0700;
		++col;
		if(col == 80)
			row++,col = 0;
	}
	else if(ch =='\n')
	{
		row++,col = 0;
	}
	else if(ch == '\t')
	{
		col = (col/4+1)*4;
		if(col == 80)
			row++,col = 0;
	}
	if(row == 25)
	{
		memcpy(charbase,charbase + 80,24*80*sizeof(uint32));
		memset(charbase + 24*80,0,80*sizeof(uint32));
		row = 24;
	}
	showpointer();
	return 0;
}

void kbdhandler(void) 
{
	showpointer();
	struct	dentry	*devptr;
	devptr = (struct dentry *) &devtab[CONSOLE];
	static uint32 shift;
	static uint8 *charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};
	uint32 st, data, c;
	/* 从0x64端口读取数据，若位0≠1，表明无数据可读 */
	st = inb(KBSTATP);
	if ((st & KBS_DIB) == 0)
		return;
	/* 从0x60端口读出数据（接通码或断开码） */
	data = inb(KBDATAP);
	/* 若是0xE0，说明接下来是扩展键，将shift的位7置1 */
	if (data == 0xE0) {
		shift |= E0ESC;
		return;
	}
	else if (data & 0x80) {
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return;
	}
	/* 上一次接收到的扫描码是E0：
	* 将此处的扫描码最高位置1，即改为断开码形式。
	* （normalmap、shiftmap、ctlmap等对于要处理的扩展键都使用断开码索引）
	* 消除shift的最高位，对后续获取的按键表明E0扫描码已处理（如果此次是扩展键按下，下次
	* 放开时，会有另一个E0）。
	* 继续向下执行其他操作，如：获取扫描码对应的字符ASCII码。
	*/
	else if (shift & E0ESC){
		data |= 0x80;
		shift &= ~E0ESC;
	}
	/* 根据前边得到的接通码（原始扫描码或是扩展键的第二字节断开码），从shiftcode中获得键码，
	* 与shift做或操作，即：如果按键是CTL、SHIFT、ALT，通过shift记住这些按键（可组合）。
	*/
	shift |= shiftcode[data];
	/* 根据前边得到的接通码（原始扫描码或是扩展键的第二字节断开码），从togglecode中获得键码，
	* 与shift做异或操作，即：如果按键是大写锁定键、数字锁定键、滚动锁定键，通过shift记住
	* 这些按键（可组合）：
	* - 原有的此类键被消除（如：此前按过CapsLock，这里再按一次，则取消大写锁定）
	*/
	shift ^= togglecode[data];
	/* 将shift与（CTL | SHIFT）做与操作：如果shift已经记录了CTL或是SHIFT按键之一，则取
	* charcode的第2个或第1个元素，分别是ctlmap、shiftmap；如果shift同时记录了这两者，则
	* 取charcode的第3个，即ctlmap（同时按下Ctrl+Shift，忽略Shift）；否则，没有按下Ctrl
	* 或Shift，使用normalmap。
	* 从刚获得的map中，根据扫描码得到相应的ASCII码；如果对应的ASCII码为0，忽略后续的操作。
	*/
	c = charcode[shift & (CTL | SHIFT)][data];
	if (!c) return;
	/* 根据前边得到的ASCII字符，如果是BACKSPACE键：
	* 如果内核的输入缓冲区中，当前行存在输入数据（行内光标>0），将光标后退一格，擦除屏幕上
	* 相应的字符。
	*/
	if (c == TY_BACKSP) {
		if (kbdcb.tyicursor > 0) {
			kbdcb.tyicursor--;
			--kbdcb.tyitail;
			fb_erase1(); /* 擦除前一个字符（略） */
			if(*kbdcb.tyitail< TY_BLANK || *kbdcb.tyitail == 0177)
				fb_erase1();

		}
		return;
	}
	/* 如果是回车或换行键：
	* 在屏幕上输出换行（开始一个新行）；将字符写入内核的输入缓冲区（由上层应用程序决定如何
	* 处理）；移动缓冲区的尾指针。
	* 输入回车，认为命令输入完毕，整行都将交由上层应用程序处理，所以将行内光标置0，通知在
	* 缓冲区信号量上等待的进程，有icursor+1个可用数据。
	*/
	else if (c == TY_NEWLINE || c == TY_RETURN) {
		int32 icursor = kbdcb.tyicursor;
		fbputc(devptr, TY_NEWLINE); /* 在显示器上显示字符（换行） */
		*(kbdcb.tyitail++) = c;
		if (kbdcb.tyitail >= &kbdcb.tyibuff[TY_IBUFLEN]) {
			kbdcb.tyitail = kbdcb.tyibuff;
		}
		kbdcb.tyicursor = 0;
		signaln(kbdcb.tyisem, icursor + 1);
		return;
	} 
	/* 检查当前输入缓冲区中有多少个字符：
	* 如果有进程在等待输入，则缓冲区中可能有字符，但尚未输入回车————暂认为缓冲区无字符。
	* 若缓冲区有一些陈旧的字符，但此时的输入是这些陈旧字符之后开启的新行，确保缓冲区未溢出。
	* （可能的场景：未有应用程序在等待读入数据，但用户输入了含回车在内的很多字符）
	*/
	int32 avail = semcount(kbdcb.tyisem);
	if (avail < 0) avail = 0;
	if ((avail + kbdcb.tyicursor) >= TY_IBUFLEN - 1) {
		return;
	}
	/* 如果此前按下了CapsLock键：对英文字符做大小写转换。 */
	if (shift & CAPSLOCK) {
		if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if ('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}
	/* 如果是不可打印字符（ASCII码空格之前或是DEL键0x7F），以形如^A的形式在屏幕上显示：
	* 如Ctrl+A显示为^A。
	* 否则，直接将字符在屏幕上输出。
	*/
	if (c < TY_BLANK || c == 0177) {
		fbputc(devptr, TY_UPARROW);
		fbputc(devptr, c + 0100);
	} else {
		fbputc(devptr, c);
	}

	*kbdcb.tyitail++ = c;
	kbdcb.tyicursor++;
	if (kbdcb.tyitail >= &kbdcb.tyibuff[TY_IBUFLEN]) {
		kbdcb.tyitail = kbdcb.tyibuff;
	}

}
