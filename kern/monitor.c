// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

static int
runcmd(char *buf, struct Trapframe *tf);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace information(function name line parameter) ", mon_backtrace },
	{ "time", "Report time consumed by pipeline's execution.", mon_time },
	{ "c", "GDB-style instruction continue.", mon_c },
	{ "si", "GDB-style instruction stepi.", mon_si },
	{ "x", "GDB-style instruction examine.", mon_x },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr)); 
    return pretaddr;
}

void
do_overflow(void)
{
    cprintf("Overflow success\n");
}

void
start_overflow(void)
{
    char * pret_addr = (char *) read_pretaddr();
    uint32_t overflow_addr = (uint32_t) do_overflow;
    int i;
    for (i = 0; i < 4; ++i)
      cprintf("%*s%n\n", pret_addr[i] & 0xFF, "", pret_addr + 4 + i);
    for (i = 0; i < 4; ++i)
      cprintf("%*s%n\n", (overflow_addr >> (8*i)) & 0xFF, "", pret_addr + i);
}

void
overflow_me(void)
{
        start_overflow();
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    overflow_me();
    cprintf("Stack backtrace:\n");
    uint32_t * ebp = (uint32_t *) read_ebp();
    while (ebp != NULL) {
      uint32_t eip = ebp[1];
      cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n",
          eip, (uint32_t)ebp, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);

      struct Eipdebuginfo info;
      debuginfo_eip((uintptr_t)eip, &info);
      cprintf("         %s:%u %.*s+%u\n",
          info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - (uint32_t)info.eip_fn_addr);
      ebp = (uint32_t *) (*ebp);
    }
    cprintf("Backtrace success\n");
	return 0;
}

// time implement
int
mon_time(int argc, char **argv, struct Trapframe *tf){
  const int BUFLEN = 1024;
  char buf[BUFLEN];
  int bufi=0;
  int i;
  for(i=1;i<argc;i++){
    char * argi =argv[i];
    int j,ch;
    for(j=0,ch=argi[0];ch!='\0';ch=argi[++j]){
      buf[bufi++]=ch;
    }
    if(i == argc-1){
      buf[bufi++]='\n';
      buf[bufi++]='\0';
      break;
    }else{
      buf[bufi++]=' ';
    }
  }

  unsigned long eax, edx;
  __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
  unsigned long long timestart  = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

  runcmd(buf, NULL);

  __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
  unsigned long long timeend  = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

  cprintf("kerninfo cycles: %08d\n",timeend-timestart);
  return 0;
}

//continue
int
mon_c(int argc, char **argv, struct Trapframe *tf){
  if(tf){//GDB-mode
    tf->tf_eflags &= ~FL_TF;
    return -1;
  }
  cprintf("not support continue in non-gdb mode\n");
  return 0;
}

//stepi
int
mon_si(int argc, char **argv, struct Trapframe *tf){
  if(tf){//GDB-mode
    tf->tf_eflags |= FL_TF;
    struct Eipdebuginfo info;
    debuginfo_eip((uintptr_t)tf->tf_eip, &info);
    cprintf("tf_eip=%08x\n%s:%u %.*s+%u\n",
        tf->tf_eip,info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, tf->tf_eip - (uint32_t)info.eip_fn_addr);
    return -1;
  }
  cprintf("not support stepi in non-gdb mode\n");
  return 0;
}

//examine
int
mon_x(int argc, char **argv, struct Trapframe *tf){
  if(tf){//GDB-mode
    if (argc != 2) {
      cprintf("Please enter the address");
      return 0;
    }
    uintptr_t examine_address = (uintptr_t)strtol(argv[1], NULL, 16);
    uint32_t examine_value;
    __asm __volatile("movl (%0), %0" : "=r" (examine_value) : "r" (examine_address));
    cprintf("%d\n", examine_value);
    return 0;
  }
  cprintf("not support stepi in non-gdb mode\n");
  return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
