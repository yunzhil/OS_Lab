/* scheduler.c */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "util.h"
#include "queue.h"

int scheduler_count;
// process or thread runs time
uint64_t cpu_time;

void printstr(char *s);
void printnum(unsigned long long n);
void scheduler(void)
{
	++scheduler_count;

	// pop new pcb off ready queue
	/* need student add */ 
	current_running=queue_pop(ready_queue);
	(*current_running).state=PROCESS_RUNNING;
}

void do_yield(void)
{
	save_pcb();
	/* push the qurrently running process on ready queue */
	/* need student add */
	queue_push(ready_queue,current_running);
	(*current_running).state = PROCESS_READY;

	// call scheduler_entry to start next task
	scheduler_entry();

	// should never reach here
	ASSERT(0);
}

void do_exit(void)
{
	/* need student add */
	(*current_running).state=PROCESS_EXITED;
	scheduler_entry();
}

void block(void)
{
	save_pcb();
	/* need student add */
	queue_push(blocked_queue,current_running);
	(*current_running).state = PROCESS_BLOCKED;
	scheduler_entry();

	// should never reach here
	ASSERT(0);
}

int unblock(void)
{
	/* need student add */
	pcb_t *temp;
	if(blocked_tasks())
	{
		temp = queue_pop(blocked_queue);
		(*temp).state = PROCESS_READY;
		queue_push(ready_queue, temp);
	}
}

bool_t blocked_tasks(void)
{
	return !blocked_queue->isEmpty;
}
