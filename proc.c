#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#ifdef CS333_P2
#include "uproc.h"
#endif

static char *states[] = {
[UNUSED]    "unused",
[EMBRYO]    "embryo",
[SLEEPING]  "sleep ",
[RUNNABLE]  "runble",
[RUNNING]   "run   ",
[ZOMBIE]    "zombie"
};


#ifdef CS333_P3
#define statecount NELEM(states)
struct ptrs {
  struct proc * head;
  struct proc * tail;
};
#endif

static struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3
  struct ptrs list[statecount];
#endif
} ptable;

static struct proc *initproc;

uint nextpid = 1;
extern void forkret(void);
extern void trapret(void);
static void wakeup1(void* chan);
#ifdef CS333_P3
static void initProcessLists(void);
static void initFreeList(void);
static void stateListAdd(struct ptrs*, struct proc*);
static void assertState(struct proc*, enum procstate);
static int  stateListRemove(struct ptrs*, struct proc* p);
int ChangeState(struct proc *p, enum procstate from, enum procstate to);
int searchList(int pid, enum procstate list_name);
//static void promoteAll();

#endif
void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid) {
      return &cpus[i];
    }
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
#ifdef CS333_P3 // MODIFIED ALLOC PROC
allocproc(void)
{
  struct proc *p;
  char *sp;
  acquire(&ptable.lock);
  p = ptable.list[UNUSED].head; 
  
  int rv = stateListRemove(&ptable.list[p->state], p);
  if(rv < 0) {
    release(&ptable.lock);
    return 0;
  } 
  assertState(p, UNUSED);
  p->state = EMBRYO;
  stateListAdd(&ptable.list[p->state], p);
  
#ifdef CS333_P2
  p->cpu_ticks_in = 0;
  p->cpu_ticks_total = 0;
  p->pid = nextpid++;
#endif
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){

  
    acquire(&ptable.lock);	
    int rv = stateListRemove(&ptable.list[p->state], p);
    if(rv < 0) {
      release(&ptable.lock);
      return 0;
    } 
	
	assertState(p, EMBRYO);
    p->state = UNUSED;
	stateListAdd(&ptable.list[p->state], p);
	release(&ptable.lock);
    return 0;

  }

  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->start_ticks = ticks;
  return p;
}
#else

allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  int found = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED) {
      found = 1;
      break;
    }
  if (!found) {
    release(&ptable.lock);
    return 0;
  }
  p->state = EMBRYO;
#ifdef CS333_P2
  p->cpu_ticks_in = 0;
  p->cpu_ticks_total = 0;
  p->pid = nextpid++;
#endif
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->start_ticks = ticks;
  return p;
}
#endif // ALLOC PROC
//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
#ifdef CS333_P3
  initProcessLists();
  initFreeList();
#endif
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S
  #ifdef CS333_P1
  p->uid = DEFAULTUID;
  p->gid = DEFAULTGID;
  #endif
  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  #ifdef CS333_P3

  acquire(&ptable.lock);
  ChangeState(p, EMBRYO, RUNNABLE);
  release(&ptable.lock);
  #else 
  acquire(&ptable.lock);
  p->state = RUNNABLE;
  release(&ptable.lock);
  #endif
  
}
// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i;
  uint pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  
  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
  acquire(&ptable.lock);
  #ifdef CS333_P3
  assertState(np, EMBRYO);
  stateListRemove(&ptable.list[np->state], np);
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
  stateListAdd(&ptable.list[np->state], np);
  release(&ptable.lock);
  #else
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    release(&ptable.lock);
  #endif
  return -1;
  }
  np->sz = curproc->sz;
  #ifdef CS333_P2
  np->gid = curproc->gid;
  np->uid = curproc->uid;
  #endif //P2
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;
  #ifdef CS333_P3
  acquire(&ptable.lock);
  int rv = stateListRemove(&ptable.list[np->state], np);
  if(rv < 0) {
      release(&ptable.lock);
	  panic("ChangeState function could not remove from list");
    } 
  assertState(np, EMBRYO);
  np->state = RUNNABLE;
  stateListAdd(&ptable.list[np->state], np);
  release(&ptable.lock);
  //ChangeState(np, EMBRYO, RUNNABLE);
  #else
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  #endif

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifdef CS333_P3
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  enum procstate from = curproc->state;
  ChangeState(curproc, from, ZOMBIE);
  sched();
  panic("zombie exit");
}
#else

void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#endif
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
/*
#ifdef CS333_P3 // P3 Altered Scheduler function
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
	p = ptable.list[RUNNABLE].head;
	while (p) {

    int rv = stateListRemove(&ptable.list[p->state], p);
    if(rv < 0) {
      release(&ptable.lock);
	  panic("ChangeState function could not remove from list");
    } 
      assertState(p, RUNNABLE);
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
      switchuvm(p);
#ifdef CS333_P2
	  p->cpu_ticks_in = ticks;		// KT 10.12
#endif
      p->state = RUNNING;
      stateListAdd(&ptable.list[RUNNING], p);
      swtch(&(c->scheduler), p->context);
      switchkvm();
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
	  p = ptable.list[RUNNABLE].head;
  }

    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}

#else // ORIGINAL SCHED FUNCTION
*/
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
#ifdef CS333_P2
	  p->cpu_ticks_in = ticks;		// KT 10.12
#endif
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}
//#endif // SCHEDULER


// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  p->cpu_ticks_total = ticks - p->cpu_ticks_in;		// KT 10.12
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
  curproc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
#ifdef CS333_P3
static void
wakeup1(void *chan)
{
  struct proc *p;
  p = ptable.list[SLEEPING].head;
  while(p) {
    if(p->state == SLEEPING && p->chan == chan) {
	  ChangeState(p, SLEEPING, RUNNABLE);
	  }
	  p = ptable.list[SLEEPING].head->next;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifdef CS333_P3
int
kill(int pid)
{
  acquire(&ptable.lock);
  int found = 0;
  int end = 0;
  while(found == 0 && end != 1) {
  if((found = searchList(pid, RUNNING)) == 1)
    break;
  if((found = searchList(pid, RUNNABLE)) == 1)
    break;
  if((found = searchList(pid, EMBRYO)) == 1)
    break;
  if((found = searchList(pid, SLEEPING)) == 1)
    break;
  end = 1;
	  }
  release(&ptable.lock);

  if(found == 0)
    return -1;
  return 0;


}
int searchList(int pid, enum procstate list_name) {

  struct proc *p = ptable.list[list_name].head;
  while(p) {
    if(p->pid == pid) {
	  p->killed = 1;
      if(p->state == SLEEPING)
	    ChangeState(p, SLEEPING, RUNNABLE); 
      release(&ptable.lock);
	  return 1;
	  }
	  p = ptable.list[list_name].head->next;
	}
	return 0;
}
#else
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif
//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
/*
void
convert_ticks(proc *&p)
{
  struct proc *p = myproc();
  char *state;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
  int time_elapsed = ticks - p->start_ticks;
  int ex_ms = time_elapsed%1000;
  int lhs = 0; 
  if(time_elapsed >=1000) 
  {
    lhs = time_elapsed - ex_ms; 
    lhs = lhs/1000;
  }
  if(ex_ms < 10)
    cprintf("%d\t%s\t%d.00%d\t%s\t%d\t", p->pid, p->name, lhs, ex_ms, state, p->sz);
  else if(ex_ms < 100)
	cprintf("%d\t%s\t%d.0%d\t%s\t%d\t", p->pid, p->name, lhs, ex_ms, state, p->sz);
  else
	cprintf("%d\t%s\t%d.%d\t%s\t%d\t", p->pid, p->name, lhs, ex_ms, state, p->sz);

}*/
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  cprintf("PID\tName\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\tPCs\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
	  
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    // Converting ticks to seconds and milliseconds
	int ppid;
    if(!p->parent)	
	  ppid = p->pid;
	else
	  ppid = p->parent->pid; 

    int time_elapsed = ticks - p->start_ticks;
    int ex_ms = time_elapsed%1000;
    int lhs = 0; 
    if(time_elapsed >=1000) {
      lhs = time_elapsed - ex_ms; 
      lhs = lhs/1000;
      }
    if(ex_ms < 10)
      cprintf("%d\t%s\t%d\t%d\t%d\t%d.00%d\t%d\t%s\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, lhs, ex_ms, p->cpu_ticks_total, state, p->sz);
    else if(ex_ms < 100)
      cprintf("%d\t%s\t%d\t%d\t%d\t%d.0%d\t%d\t%s\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, lhs, ex_ms, p->cpu_ticks_total, state, p->sz);
    else
      cprintf("%d\t%s\t%d\t%d\t%d\t%d.%d\t%d\t%s\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, lhs, ex_ms, p->cpu_ticks_total, state, p->sz);

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef CS333_P2
int
getprocs (uint x, struct uproc * table)
{

  struct proc *p;
  int n = 0;
  char *state;
 if(x > 64)
 	x = 64;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC] && n < x; p++){
    if(p->state == RUNNING || p->state == RUNNABLE || p->state == SLEEPING || p->state == ZOMBIE)
	{  
	  // Set enum state to char array
      if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
      else
        state = "???";

	  // Setting pid
	  if(!p->parent)
	  	table[n].ppid = p->pid;
	  else
	    table[n].ppid = p->parent->pid;

	  table[n].pid = p->pid;
      safestrcpy(table[n].name, p->name, sizeof(p->name));
	  table[n].uid = p->uid;
	  table[n].gid = p->gid;
	  table[n].elapsed_ticks = ticks - p->start_ticks;
	  table[n].CPU_total_ticks= p->cpu_ticks_total;
	  safestrcpy(table[n].state, state, sizeof(state));
	  table[n].size = p->sz;
	  n++;
	}
	else
      continue;
	}
	
	  release(&ptable.lock);
  if(n > 0)
    return n;
  return -1;
}
#endif

int
setuid (uint n)
{
  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->uid = n;
  release(&ptable.lock);
  return 0;
}

int 
setgid (uint n)
{
  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->gid = n;
  release(&ptable.lock);
  return 0;
}


// list management function prototypes


// list management helper functions
static void
stateListAdd(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL){
    (*list).head = p;
    (*list).tail = p;
    p->next = NULL;
  } else{
    ((*list).tail)->next = p;
    (*list).tail = ((*list).tail)->next;
    ((*list).tail)->next = NULL;
  }
}

static int
stateListRemove(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL || (*list).tail == NULL || p == NULL){
    return -1;
  }

  struct proc* current = (*list).head;
  struct proc* previous = 0;

  if(current == p){
    (*list).head = ((*list).head)->next;
    // prevent tail remaining assigned when we've removed the only item
    // on the list
    if((*list).tail == p){
      (*list).tail = NULL;
    }
    return 0;
  }

  while(current){
    if(current == p){
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found. return error
  if(current == NULL){
    return -1;
  }

  // Process found.
  if(current == (*list).tail){
    (*list).tail = previous;
    ((*list).tail)->next = NULL;
  } else{
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = NULL;

  return 0;
}

static void
initProcessLists()
{
  int i;

  for (i = UNUSED; i <= ZOMBIE; i++) {
    ptable.list[i].head = NULL;
    ptable.list[i].tail = NULL;
  }
#ifdef CS333_P4
  for (i = 0; i <= MAXPRIO; i++) {
    ptable.ready[i].head = NULL;
    ptable.ready[i].tail = NULL;
  }
#endif
}

static void
initFreeList(void)
{
  struct proc* p;

  for(p = ptable.proc; p < ptable.proc + NPROC; ++p){
    p->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED], p);
  }
}

static void 
assertState(struct proc* p, enum procstate assumed) {
   
  if(p->state == assumed)
    return;
  panic("Pstate not as expected");
}

#ifdef CS333_P3
int ChangeState(struct proc *p, enum procstate from, enum procstate to) {

  int rv = stateListRemove(&ptable.list[p->state], p);
  if(rv < 0) {
      release(&ptable.lock);
	  panic("ChangeState function could not remove from list");
    } 
  assertState(p, from);
  p->state = to;
  stateListAdd(&ptable.list[p->state], p);
  return 0;
}

#endif


