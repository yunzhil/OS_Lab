/* Author(s): <Your name here>
 * COS 318, Fall 2013: Project 3 Pre-emptive Scheduler
 * Implementation of the process scheduler for the kernel.
 */

#include "common.h"
#include "interrupt.h"
#include "queue.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"
#include "syslib.h"

priority_t original_priority[MAX_NUM];
priority_t running_priority[MAX_NUM];

pcb_t *current_running;
node_t ready_queue;
node_t sleep_wait_queue;
// more variables...
volatile uint64_t time_elapsed;

/* TODO:wake up sleeping processes whose deadlines have passed */
void check_sleeping(){
  uint32_t now = do_gettimeofday();
  node_t *tmp;
  while(!is_empty(&sleep_wait_queue)&&((pcb_t*)peek(&sleep_wait_queue))->deadline <= now)
  {
    tmp = dequeue(&sleep_wait_queue);
    enqueue(&ready_queue,tmp);
    ((pcb_t *)tmp)->status = READY;
  }
}

/* Round-robin scheduling: Save current_running before preempting */
void put_current_running(){
  enqueue(&ready_queue, (node_t *) current_running);
  current_running->status = READY;
}

/* Change current_running to the next task */
void scheduler(){
     ASSERT(disable_count);
     check_sleeping(); // wake up sleeping processes
     while (is_empty(&ready_queue)){
          leave_critical();
          enter_critical();
          check_sleeping();
     }
     current_running = (pcb_t *) dequeue(&ready_queue);
     pcb_t *record = current_running;
     pcb_t *temp = record;
     do{
       enqueue(&ready_queue,(node_t*)current_running);
       current_running = (pcb_t*)dequeue(&ready_queue);
       if(running_priority[current_running->pid]>running_priority[temp->pid])
       {
         temp = current_running;
       }
     }while(current_running!=record);
     if(running_priority[temp->pid]<0)
     {
       do{
         enqueue(&ready_queue,(node_t*)current_running);
         current_running = (pcb_t*)dequeue(&ready_queue);
         running_priority[current_running->pid] = original_priority[current_running->pid];
       }while(record!=current_running);
     
     record = current_running;
     temp = record;
      do{
       enqueue(&ready_queue,(node_t*)current_running);
       current_running = (pcb_t*)dequeue(&ready_queue);
       if(running_priority[current_running->pid]>running_priority[temp->pid])
       {
         temp = current_running;
       }
     }while(current_running!=record);
     }
     if (current_running!=temp){
       //enqueue(&ready_queue,(node_t*)current_running);
       temp->node.prev->next = (node_t*)current_running;
       temp->node.next->prev = (node_t*)current_running;
       current_running->node.prev = temp->node.prev;
       current_running->node.next = temp->node.next;
       current_running = temp;
     }
     else{
       ;
     }

     running_priority[current_running->pid]--;
     ASSERT(NULL != current_running);
     ++current_running->entry_count;
     // check type
   //  if(current_running->task_type == PROCESS)
   //  current_running->nested_count--;  
}

int lte_deadline(node_t *a, node_t *b) {
     pcb_t *x = (pcb_t *)a;
     pcb_t *y = (pcb_t *)b;

     if (x->deadline <= y->deadline) {
          return 1;
     } else {
          return 0;
     }
}

void do_sleep(int milliseconds){
     ASSERT(!disable_count);

     enter_critical();
     // TODO
     current_running->status = SLEEPING;
     current_running->deadline = do_gettimeofday() + milliseconds;
     sort_and_enqueue(&sleep_wait_queue, (node_t *) current_running, lte_deadline);
     scheduler_entry();
     leave_critical();
}

void do_yield(){
     enter_critical();
     put_current_running();
     scheduler_entry();
     leave_critical();
}

void do_exit(){
     enter_critical();
     current_running->status = EXITED;
     scheduler_entry();
     /* No need for leave_critical() since scheduler_entry() never returns */
}

void block(node_t * wait_queue){
     ASSERT(disable_count);
     current_running->status = BLOCKED;
     enqueue(wait_queue, (node_t *) current_running);

     enter_critical();
     scheduler_entry();
     leave_critical();
}

void unblock(pcb_t * task){
     ASSERT(disable_count);
     task->status = READY;
     enqueue(&ready_queue, (node_t *) task);
}

pid_t do_getpid(){
     pid_t pid;
     enter_critical();
     pid = current_running->pid;
     leave_critical();
     return pid;
}

uint64_t do_gettimeofday(void){
     return time_elapsed;
}

priority_t do_getpriority(){
	/* TODO */
  return original_priority[current_running->pid];
}


void do_setpriority(priority_t priority){
	/* TODO */
  original_priority[current_running->pid] = priority;
  running_priority[current_running->pid] = priority;
}

uint64_t get_timer(void) {
     return do_gettimeofday();
}
