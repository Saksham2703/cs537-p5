#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "mmap.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    /* CASE - MAP_GROWSUP*/
    void* curaddr = (void*) rcr2();
    for (int i = 0; i < 32; i++) {
      // cprintf("i:%d\tvalid:%d\tstart_addr:0x%x\tend_addr:0x%x\n", i, myproc()->va[i].valid, myproc()->va[i].start_ad, myproc()->va[i].end_ad);

      if (myproc()->va[i].valid == 0) {
        cprintf("Segmentation Fault\n");
        // kill the process
        myproc()->killed = 1;
        break;
      }

      if (myproc()->va[i].end_ad < curaddr && (myproc()->va[i].end_ad + PGSIZE) > curaddr) {
        if ((myproc()->va[i].flags & MAP_GROWSUP) == MAP_GROWSUP) {
          if (i != 31 && myproc()->va[i + 1].valid == 1 && ((myproc()->va[i + 1].start_ad - myproc()->va[i + 1].end_ad) < (2 * PGSIZE))) {
            cprintf("Segmentation Fault\n");
            // kill the process
            myproc()->killed = 1;
            break;
          } else {
            char *mem;

            mem = kalloc();
            if(mem == 0) {
              // kill the process
              myproc()->killed = 1;
              break;
            }

            memset(mem, 0, PGSIZE);
            if (mappages(myproc()->pgdir, (myproc()->va[i].end_ad + 1), PGSIZE, V2P(mem), myproc()->va[i].prot | PTE_U) == -1) {
              cprintf("Mappages returned -1.\n");
              // kill the process
              myproc()->killed = 1;
              break;
            }

            myproc()->va[i].end_ad += PGSIZE;
            myproc()->va[i].len += PGSIZE;
            break;
          }
        } else {
          cprintf("Segmentation Fault\n");
          // kill the process
          myproc()->killed = 1;
          break;
        }
      }
    }
    
    break;


  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
