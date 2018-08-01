/* Simple counter program, to check the functionality of do_yield(). Print
   time in seconds.

   Best viewed with tabs set to 4 spaces. */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"
#include "printf.h"

/*
 * This thread runs indefinitely, which means that the
 * scheduler should never run out of processes.
 */
void thread_1(void)
{
    priority_t priority = 1;
    do_setpriority(priority);
    printf(4, 10, "Priority of thread 1 is %d", do_getpriority());
    int i=0;
    while (TRUE) {
        i++;
        printf(5, 10, "Thread 1 executions : %d", i);
        do_yield();
    }
}

void thread_2(void)
{
    priority_t priority = 5;
    do_setpriority(priority);
    printf(7, 10, "Priority of thread 2 is %d", do_getpriority());
    int i=0;
    while (TRUE) {
        i++;
        printf(8, 10, "Thread 2 executions : %d", i);
        do_yield();
    }
}

void thread_3(void)
{
    priority_t priority = 9;
    do_setpriority(priority);
    printf(10, 10, "Priority of thread 3 : %d", do_getpriority());
    int i=0;
    while (TRUE) {
        i++;
        printf(11, 10, "Thread 3 executions : %d", i);
        do_yield();
    }
}
