/* tasks.c: the list of threads and processes to run (DO NOT CHANGE) */

#include "scheduler.h"
#include "th.h"

struct task_info task[] = {
    /* clock and status thread */
    TH(&thread_1),
    TH(&thread_2),
    TH(&thread_3),
    };

enum {
    NUM_TASKS = sizeof task / sizeof(struct task_info)
};
