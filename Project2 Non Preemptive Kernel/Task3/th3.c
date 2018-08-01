#include "common.h"
#include "scheduler.h"
#include "util.h"

uint32_t thread4_to_5_begin;
uint32_t thread4_to_5_end;

uint32_t thread5_to_4_begin;
uint32_t thread5_to_4_end;

uint32_t tt_cost[50];
uint32_t tp_cost[50];

void thread4(void)
{
    int i=0;
    for(i=0;i<50;i++)
    {
        thread4_to_5_begin = get_timer();
        do_yield();
        thread5_to_4_end = get_timer();
        tp_cost[i] = (thread5_to_4_end - thread5_to_4_begin)/2;
    }
    do_exit();
}

void thread5(void)
{
    int i=0;
    for(i=0;i<50;i++)
    {
        thread4_to_5_end = get_timer();
        tt_cost[i] = thread4_to_5_end - thread4_to_5_begin;
        thread5_to_4_begin = get_timer();
        do_yield();
    }
    uint32_t temp_tt = 0;
    for(i=0;i<50;i++)
    {
        temp_tt = temp_tt+tt_cost[i];
    }
    temp_tt = temp_tt/50;
    uint32_t temp_tp = 0;
    for(i=0;i<50;i++)
    {
        temp_tp = temp_tp + tp_cost[i];
    }
    temp_tp = temp_tp/50;

    print_location(0, 1);
	printstr("The Time in switch between thread and process (in us): ");
	printint(60, 1, temp_tp);

    print_location(0, 2);
	printstr("The Time in switch between thread and thread (in us): ");
	printint(60, 2, temp_tt);

    do_exit();
}
