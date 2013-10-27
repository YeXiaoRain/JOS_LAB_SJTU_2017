// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int a;
	a=10;
	cprintf("At first , a equals %d\n",a);
	cprintf("&a equals 0x%x\n",&a);
	asm volatile("int $3");
	// Try single-step here
	a=20;
	cprintf("Finally , a equals %d\n",a);
}

