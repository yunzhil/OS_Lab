/* kernel.c: Kernel with pre-emptive scheduler */
/* Do not change this file */

#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "queue.h"
#include "scheduler.h"
#include "util.h"
#include "printf.h"
#include "printk.h"
#include "ramdisk.h"
#include "mbox.h"

#include "files.h"

#define NUM_TASKS 	(32)
#define NUM_PCBS      (NUM_TASKS)

pcb_t pcb[NUM_PCBS];
condition_t wait_cond[32];

/* This is the system call table, used in interrupt.c */
int (*syscall[NUM_SYSCALLS]) ();

static uint32_t *stack_new(pid_t pid);
static void first_entry(void);
static int invalid_syscall(void);
static void init_syscalls(void);
static void init_serial(void);
static void initialize_pcb(pcb_t *p, pid_t pid, struct task_info *ti);
static int do_spawn(const char *filename);
static int do_kill(pid_t pid);
static int do_wait(pid_t pid);

extern void asm_start();

void printcharc(char ch);

void __attribute__((section(".entry_function"))) _start(void)
{
	asm_start();
	static pcb_t garbage_registers;
	int i;

	clear_screen(0, 0, 80, 30);

	queue_init(&ready_queue);
	queue_init(&sleep_wait_queue);
	current_running = &garbage_registers;

	/* Transcribe the task[] array (in tasks.c)
	 * into our pcb[] array.
	 */
	for (i = 0; i < NUM_TASKS; ++i) {
		pcb[i].status = EXITED;
		condition_init(&wait_cond[i]);
	}

	init_syscalls();
	init_interrupts();

	srand(get_timer()); /* using a random value */
	init_mbox();

	do_spawn("init");
	/* Schedule the first task */
	enter_critical();
	scheduler_entry();
	/* We shouldn't ever get here */
	ASSERT(FALSE);
}


static uint32_t *stack_new(pid_t pid)
{
	//static uint32_t next_stack = 0xa0f00000;

	//next_stack += 0x10000;
	//ASSERT(next_stack <= 0xa1000000);
	return (uint32_t *) (0xa0f00000+pid*0x10000);
}

static void initialize_pcb(pcb_t *p, pid_t pid, struct task_info *ti)
{
	p->entry_point = ti->entry_point;
	p->pid = pid;
	p->task_type = ti->task_type;
	p->priority = 1;
	p->status = FIRST_TIME;
	switch (ti->task_type) {
		case KERNEL_THREAD:
			p->kernel_tf.regs[29] = (uint32_t)stack_new(pid);
			p->nested_count = 1;
			break;
		case PROCESS:
			p->kernel_tf.regs[29] = (uint32_t)stack_new(pid);
			p->user_tf.regs[29] = (uint32_t)stack_new(pid);
			p->nested_count = 0;
			break;
		default:
			ASSERT(FALSE);
	}
	p->kernel_tf.regs[31] = (uint32_t) first_entry;
}


static void first_entry()
{
	uint32_t *stack, entry_point;

	enter_critical();

	if (KERNEL_THREAD == current_running->task_type) {
		stack = current_running->kernel_tf.regs[29];
	} else {
		stack = current_running->user_tf.regs[29];
	}
	entry_point = current_running->entry_point;

	// Messing with %esp in C is usually a VERY BAD IDEA
	// It is safe in this case because both variables are
	// loaded into registers before the stack change, and
	// because we jmp before leaving asm()
	asm volatile ("add $sp, $0, %0\n"
			"jal leave_critical\n"
			"nop\n"
			"add $ra, $0, %1\n"
			"jr $ra\n"
			:: "r" (stack), "r" (entry_point));

	ASSERT(FALSE);
}


static int invalid_syscall(void)
{
	HALT("Invalid system call");
	/* Never get here */
	return 0;
}


/* Called by kernel to assign a system call handler to the array of
   system calls. */
static void init_syscalls()
{
	int fn;

	for (fn = 0; fn < NUM_SYSCALLS; ++fn) {
		syscall[fn] = &invalid_syscall;
	}
	syscall[SYSCALL_YIELD] = (int (*)()) &do_yield;
	syscall[SYSCALL_EXIT] = (int (*)()) &do_exit;
	syscall[SYSCALL_GETPID] = &do_getpid;
	syscall[SYSCALL_GETPRIORITY] = &do_getpriority;
	syscall[SYSCALL_SETPRIORITY] = (int (*)()) &do_setpriority;
	syscall[SYSCALL_SLEEP] = (int (*)()) &do_sleep;
	syscall[SYSCALL_SHUTDOWN] = (int (*)()) &do_shutdown;
	syscall[SYSCALL_WRITE_SERIAL] = (int (*)()) &do_write_serial;
	syscall[SYSCALL_PRINT_CHAR] = (int (*)()) &print_char;
	syscall[SYSCALL_SPAWN] = (int (*)()) &do_spawn;
	syscall[SYSCALL_KILL] = (int (*)()) &do_kill;
	syscall[SYSCALL_WAIT] = (int (*)()) &do_wait;
    syscall[SYSCALL_MBOX_OPEN] = (int (*)()) &do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = (int (*)()) &do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = (int (*)()) &do_mbox_send;
    syscall[SYSCALL_MBOX_RECV] = (int (*)()) &do_mbox_recv;
	syscall[SYSCALL_TIMER] = (int (*)()) &get_timer;
}

/* Used for debugging */
void print_status(void)
{
	static char *status[] = { "First  ", "Ready", "Blocked", "Exited ", "Sleeping" };
	int i, base;

	base = 17;
	printf(base - 4, 6, "P R O C E S S   S T A T U S");
	printf(base - 2, 1, "Pid\tType\tPrio\tStatus\tEntries");
	for (i = 0; i < NUM_PCBS && (base + i) < 25; i++) {
		printf(base + i, 1, "%d\t%s\t%d\t%s\t%d", pcb[i].pid,
				pcb[i].task_type == KERNEL_THREAD ? "Thread" : "Process",
				pcb[i].priority, status[pcb[i].status], pcb[i].entry_count);
	}
}

void do_shutdown(void)
{
	/* These numbers will work for bochs
	 * provided it was compiled WITH acpi.
	 * the default ubuntu 9 version of bochs
	 * is NOT compiled with acpi support, though
	 * the version in the Friend center lab DO have it.
	 * This will probably not work with
	 * any real computer.
	 */
	//outw( 0xB004, 0x0 | 0x2000 );

	/* Failing that... */
	HALT("Shutdown");
}

#define PORT3f8 0xbfe48000
#define PORT3fd 0xbfe48006

/* Write a byte to the 0-th serial port */
void do_write_serial(int character)
{

	// wait until port is free
	unsigned long port = PORT3f8;
	int i = 50000;
	while (i--);
	*(unsigned char*)port = character;

	//leave_critical();
}

void printcharc(char ch)
{
	do_write_serial(ch);
}

int print_char(int line, int col, char c){
	unsigned long port = PORT3f8;
	print_location(line, col);

	*(unsigned char *)port = c;

}

int spawn_times = 0;
static int do_spawn(const char *filename)
{
  (void)filename;
  /* TODO */
  File* file_tmp = ramdisk_find_File(filename);
  struct task_info info_tmp;
  info_tmp.entry_point = file_tmp->process;
  info_tmp.task_type = file_tmp->task_type;
  enter_critical();
  int i=0;
  for( i=0;i<NUM_PCBS;i++)
  {
	  if (pcb[i].status == EXITED)
	  {
		  initialize_pcb(&pcb[i], i, &info_tmp);
		  enqueue(&ready_queue, (node_t *)&pcb[i]);
		  break;
	  }
  }
  leave_critical();
  return 0;
}

static int do_kill(pid_t pid)
{
  (void) pid;
  /* TODO */
  enter_critical();
  pcb[pid].status=EXITED;
  //pcb_t *record = (pcb_t*)dequeue(&ready_queue);
  search_and_remove(pid);
  leave_critical();
  condition_broadcast(& wait_cond[pid]);
  return -1;
}

static int do_wait(pid_t pid)
{
  (void) pid;
  /* TODO */

  lock_t useless;
  lock_init(&useless);
  condition_wait(&useless,&wait_cond[pid]);
  //lock_release_helper(&useless);
  return -1;
}


static int search_and_remove(pid_t pid)
{
	//remove from ready_queue
	node_t *tmp=ready_queue.next;
	while((tmp!=&ready_queue))
	{
		if(((pcb_t*)tmp)->pid==pid)
		{
			node_t *op = tmp;
			op->prev->next = op->next;
			op->next->prev = op->prev;
		}
		tmp = tmp->next;
	}

	int i=0;
	for(i=0;i<32;i++)
	{
		tmp = wait_cond[i].wait_queue.next;
		while((tmp!=&wait_cond[i].wait_queue))
	    {
		if(((pcb_t*)tmp)->pid==pid)
		{
			node_t *op = tmp;
			op->prev->next = op->next;
			op->next->prev = op->prev;
		}
		tmp = tmp->next;
	}

	return 0;


	}

}

