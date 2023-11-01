#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mmap.h"

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
	void* addr;
	int length;
	int prot;
	int flags;
	int fd;
	struct file *pf;
	int offset;

	if(argint(0, (void*)&addr) < 0){
		return -1;
	}
	if(argint(1, &length) < 0){
		return -1;
	}
	if(argint(2, &prot) < 0){
		return -1;
	}
	if(argint(3, &flags) < 0){
		return -1;
	}
	if(argfd(4, &fd, &pf) < 0){ // changed this from argfd to argint
		return -1;
	}
	if(argint(5, &offset) < 0){
		return -1;
	}

	// check for valid args
	// invalid length
	if (length <= 0) {
		return -1;
	}

	// if MAP_FIXED set
	if ((flags & MAP_FIXED) / 8 == 1) {
		// check valid addr
		if (addr < (void *)MMAPSTART || addr >= (void *)KERNBASE)
		{
	   	    return -1;
		}
		// check addr multiple of page size
		if ((uint) addr % PGSIZE != 0) {
			return -1;
		}
		// map cant be fixed and anonymous at the same time
		if((flags & MAP_ANONYMOUS) / 4 == 1){
			return -1;
		}
	}

	// if MAP_SHARED and MAP_PRIVATE set together
	if((flags & (MAP_SHARED || MAP_PRIVATE)) == 3){
		return -1;
	}

	// error if map anonymous and ?? both set

	mmap(&addr, length, prot, flags, fd, offset);
	return 0;
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

	munmap(&addr, length);
	return 0;
}

