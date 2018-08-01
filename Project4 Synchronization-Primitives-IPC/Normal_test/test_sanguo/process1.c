#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void SunQuan(void)
{
  mbox_t pub = mbox_open("SunQuan-Publish-PID");
  pid_t myPid = getpid();

  /* Send PID twice, once for LiuBei,
   * and once for the CaoCao
   */
  mbox_send(pub, &myPid, sizeof(pid_t));
  mbox_send(pub, &myPid, sizeof(pid_t));

  /* Find LiuBei's PID */
  mbox_t sub = mbox_open("LiuBei-Publish-PID");

//  printf(3,1,"pub is %d",pub);
//  printf(4,1,"sub is %d",sub);

  for(;;)
  {
    pid_t liubei = 3;
    mbox_recv(sub, &liubei, sizeof(pid_t));

    printf(1,1, "SunQuan (%d): I'm waiting for Liubei :(%d)              ", myPid,liubei);

    wait(liubei);

    printf(1,1, "SunQuan(%d): I'm coming to save you, LiuBei!", myPid);

    sleep(1000);
    spawn("LiuBei");
    mbox_send(pub, &myPid, sizeof(pid_t));
  }
}
