#include "common.h"
#include "syslib.h"
#include "util.h"

void _start(void)
{
	/* need student add */
	int i=0;
	for(i=0;i<50;i++)
		yield();
	exit();
}
