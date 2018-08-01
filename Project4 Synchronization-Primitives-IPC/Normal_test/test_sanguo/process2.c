#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void LiuBei(void)
{
//  printf(2,1,"begin open");
  mbox_t pub = mbox_open("LiuBei-Publish-PID");
//  printf(7,1,"liubei,pub is %d",pub);
  pid_t myPid = getpid();

  /* Send PID twice, once for sunquan Hood,
   * and once for the CaoCao
   */
 // printf(3,1,"begin send message liubei");
  mbox_send(pub, &myPid, sizeof(pid_t));
  mbox_send(pub, &myPid, sizeof(pid_t));

  /* Find sunquan's PID */
  mbox_t sub = mbox_open("SunQuan-Publish-PID");
  
 // printf(8,1,"liubei,sub is %d",sub);

  for(;;)
  {
    pid_t aramis ;
    mbox_recv(sub, &aramis, sizeof(pid_t));

    printf(2,1, "LiuBei(%d): I'm waiting for SunQuan : (%d)        ", myPid,aramis);

   // printf(3,1,"begin wait aramis");
    wait(aramis);

    printf(2,1, "LiuBei(%d): I'm coming to save you, SunQuan!", myPid);

    sleep(1000);
    spawn("SunQuan");
    mbox_send(pub, &myPid, sizeof(pid_t));
  }

}

