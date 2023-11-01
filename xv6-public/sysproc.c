#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mmap.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// mmap
int sys_mmap(void){
	// cprintf("in sys_mmap\n");
	void* addr;
	int length;
	int prot;
	int flags;
	int fd;
	struct file *pf;
	int offset;

	// cprintf("actual addr:%d\n", 0x60020000);
	if(argint(0, (void*)&addr) < 0){
		return -1;
	}
	cprintf("addr we got:%d\n", addr);
	// cprintf("got args 1\n");
	if(argint(1, &length) < 0){
		return -1;
	}
	// cprintf("got args 2\n");
	if(argint(2, &prot) < 0){
		return -1;
	}
	// cprintf("got args 3\n");
	if(argint(3, &flags) < 0){
		return -1;
	}
	// cprintf("got args 4\n");

	// check for valid args
	// invalid length
	if (length <= 0) {
		return -1;
	}

	// if MAP_FIXED set
	if ((flags & MAP_FIXED) == MAP_FIXED) {
		// check valid addr
		if (addr < (void *)MMAPSTART || addr >= (void *)KERNBASE)
		{
	   	    return -1;
		}
		// check addr multiple of page size
		if ((uint) addr % PGSIZE != 0) {
			return -1;
		}
	}

	// if not MAP_ANONYMOUS, get file descriptor
	if ((flags & MAP_ANONYMOUS) == 0) {
		if(argfd(4, &fd, &pf) < 0){ // changed this from argfd to argint
			return -1;
		}
	} else {
		fd = -1;
	}

	// if MAP_SHARED and MAP_PRIVATE set together
	if((flags & (MAP_SHARED | MAP_PRIVATE)) == 3){
		return -1;
	}

	// cprintf("got args 5\n");
	if(argint(5, &offset) < 0){
		return -1;
	}

	// cprintf("got all args\n");


	// error if map anonymous and ?? both set
	cprintf("addr passing in:%d\n", addr);
	return mmap(addr, length, prot, flags, fd, offset);
}

// munmap
int sys_munmap(void){
	void* addr;
	int length;

	if(argint(0, (void*)&addr) < 0){
		return -1;
	}
	if(argint(0, &length) < 0){
		return -1;
	}

	// invalid length
	if (length <= 0) {
		return -1;
	}

	// addr within bounds
	if (addr < (void *)MMAPSTART || addr >= (void *)KERNBASE){
		return -1;
	}

	return munmap(addr, length);
}

