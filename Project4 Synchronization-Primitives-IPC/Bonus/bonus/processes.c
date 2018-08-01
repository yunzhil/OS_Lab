#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void init(void)
{
    ASSERT( spawn("clock_thread") >= 0 );
    pid_t proc1, proc2;
    mbox_t pub = mbox_open("Publish-Lock");
    lock_t l;
    lock_t* ptr = &l;
    open_lock(ptr);
    mbox_send(pub, &ptr, sizeof(lock_t*));
    mbox_send(pub, &ptr, sizeof(lock_t*));
    printf(1,1, "God: Lock initial successfully, which address is %u.", (unsigned int) ptr);
    printf(2,1, "God: Creating a boy...");
    ASSERT((proc1 = spawn("process1")) >= 0 );
    sleep(2000);
    printf(6,1, "God: Creating a girl...");
    ASSERT((proc2 = spawn("process2")) >= 0 );
    sleep(2000);
    printf(9,1, "God: Let the boy go...");
    kill(proc1);
    exit();
}
