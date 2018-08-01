/*
   kernel.c
   the start of kernel
   */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"
#include "queue.h"

#include "tasks.c"

volatile pcb_t *current_running;

pcb_t pcb_table[NUM_TASKS];
struct queue ready, blocked;


queue_t ready_queue, blocked_queue;
pcb_t *ready_arr[NUM_TASKS];
pcb_t *blocked_arr[NUM_TASKS];

/*
   this function is the entry point for the kernel
   It must be the first function in the file
   */

#define PORT3f8 0xbfe48000

 void printnum(unsigned long long n)
 {
   int i,j;
   unsigned char a[40];
   unsigned long port = PORT3f8;
   i=10000;
   while(i--);

   i = 0;
   do {
   a[i] = n % 16;
   n = n / 16;
   i++;
   }while(n);

  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
      *(unsigned char*)port = 'a' + a[j] - 10;
    }else{
	*(unsigned char*)port = '0' + a[j];
   }
  }
  printstr("\r\n");
}

void _stat(void){

	/* some scheduler queue initialize */
	/* need student add */
  ready_queue = &ready;
  blocked_queue = &block;
  queue_init(ready_queue);
  ready_queue->pcbs = &ready_arr;
  ready_queue->capacity = NUM_TASKS;

  queue_init(blocked_queue);
  blocked_queue->pcbs = &blocked_arr;
  blocked_queue->capacity = NUM_TASKS;



	clear_screen(0, 0, 30, 24);

	/* Initialize the PCBs and the ready queue */
	/* need student add */

  int i=0;
  for(i=0;i<NUM_TASKS;i++){
    pcb_table[i].pid = i;
    pcb_table[i].state = PROCESS_READY;
    pcb_table[i].stack_top = STACK_MIN + i*STACK_SIZE;
    pcb_table[i].stack_size = STACK_SIZE;
    pcb_table[i].task_type = task[i]->task_type;
    pcb_table[i].context.reg[29] = pcb_table[i].stack_top;
    pcb_table[i].context.reg[31] = task[i]->entry_point;
    queue_push(ready_queue,pcb_table+i); 
  }

	/*Schedule the first task */
	scheduler_count = 0;
	scheduler_entry();

	/*We shouldn't ever get here */
	ASSERT(0);
}
