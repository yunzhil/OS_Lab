#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void process2(void) {
    pid_t myPid = getpid();
    printf(7,1, "The girl has been created.Pid is : %d", myPid);
    mbox_t sub = mbox_open("Publish-Lock");
    lock_t* ptr;
    mbox_recv(sub, &ptr, sizeof(lock_t*));
    printf(8,1, "Girl : looking for  ring (lock) which address is  %u.", (unsigned int) ptr);
    acquire(ptr);
    printf(10,1, "Girl: ring acquired successfully!!! But where are you?");
    release(ptr);
    exit();
}
