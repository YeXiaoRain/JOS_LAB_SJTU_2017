#include <inc/lib.h>

#define ALLOCATE_SIZE 4096
#define STRING_SIZE	  64

void
umain(int argc, char **argv)
{
	int i;
	uint32_t start, end;
	char *s;

	start = sys_sbrk(0);
	end = sys_sbrk(ALLOCATE_SIZE);

	if (end - start < ALLOCATE_SIZE) {
		cprintf("sbrk not correctly implemented\n");
	}

	s = (char *) start;
	for ( i = 0; i < STRING_SIZE; i++) {
		s[i] = 'A' + (i % 26);
	}
	s[STRING_SIZE] = '\0';

	cprintf("SBRK_TEST(%s)\n", s);
}

