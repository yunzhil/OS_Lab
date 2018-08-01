#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void process1(void) {
    pid_t myPid = getpid();
    printf(3,1, "The boy  has been created.Pid: %d", myPid);
    mbox_t sub = mbox_open("Publish-Lock");
    lock_t* ptr;
    mbox_recv(sub, &ptr, sizeof(lock_t*));
    printf(4,1, "Boy: Acquiring the ring (lock) which address is  %u.", (unsigned int) ptr);
    acquire(ptr);
    printf(5,1, "Boy: ring acquired successfully. Where is my girl ?");
    while(1);
}
