#include "common.h"
#include "mbox.h"
#include "sync.h"
#include "scheduler.h"


typedef struct
{
	/* TODO */
  char message[MAX_MESSAGE_LENGTH];
} Message;

typedef struct
{
	/* TODO */
  char name[MBOX_NAME_LENGTH];
  Message messages[MAX_MBOX_LENGTH];
  unsigned int use_count;
  semaphore_t full;
  semaphore_t empty;
  lock_t mutex;
  int send;
  int receive;
} MessageBox;


static MessageBox MessageBoxen[MAX_MBOXEN];
lock_t BoxLock;

/* Perform any system-startup
 * initialization for the message
 * boxes.
 */
void init_mbox(void)
{
	/* TODO */
  enter_critical();
  int i=0;
  for(i=0;i<MAX_MBOXEN;i++)
  {
    char ini[] = "initial";
    bcopy(ini,MessageBoxen[i].name,8*sizeof(char));
    bzero((char*)MessageBoxen[i].messages,MAX_MBOX_LENGTH*MAX_MESSAGE_LENGTH*sizeof(char));
    MessageBoxen[i].use_count = 0;
    MessageBoxen[i].full.value = 0;
    MessageBoxen[i].empty.value = MAX_MBOX_LENGTH;
    queue_init(&MessageBoxen[i].empty.wait_queue);
    queue_init(&MessageBoxen[i].full.wait_queue);
    lock_init(&MessageBoxen[i].mutex);
    MessageBoxen[i].send=0;
    MessageBoxen[i].receive=0;
  }
  leave_critical();
}

/* Opens the mailbox named 'name', or
 * creates a new message box if it
 * doesn't already exist.
 * A message box is a bounded buffer
 * which holds up to MAX_MBOX_LENGTH items.
 * If it fails because the message
 * box table is full, it will return -1.
 * Otherwise, it returns a message box
 * id.
 */
mbox_t do_mbox_open(const char *name)
{
  (void)name;
	/* TODO */
  enter_critical();
  int i=0;
  for(i=0;i<MAX_MBOXEN;i++)
  {
    if(same_string(name,MessageBoxen[i].name))
  //  enter_critical();
   { 
     MessageBoxen[i].use_count++;
     leave_critical();
//     printf(5,1,"old one");
     return i;
   }
  }


  for(i=0;i<MAX_MBOXEN;i++)
  {
    if(MessageBoxen[i].use_count==0)
    {
     // enter_critical();
      int length = strlen(name);
      bcopy(name,MessageBoxen[i].name,length*sizeof(char));
      MessageBoxen[i].use_count++;
      leave_critical();
  //    printf(6,1,"new one");
      return i;
    }
  }
  leave_critical();
  return -1;

}

/* Closes a message box
 */
void do_mbox_close(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
  enter_critical();
  MessageBoxen[mbox].use_count--;
  if(MessageBoxen[mbox].use_count<=0)
  {
    char ini[8] = "initial";
    bcopy(ini,MessageBoxen[mbox].name,8*sizeof(char));
    MessageBoxen[mbox].use_count=0;
    semaphore_init(&MessageBoxen[mbox].full,0);
    semaphore_init(&MessageBoxen[mbox].empty,MAX_MESSAGE_LENGTH);
    lock_init(&MessageBoxen[mbox].mutex);
    MessageBoxen[mbox].send=0;
    MessageBoxen[mbox].receive=0;
    
  }
  leave_critical();
}

/* Determine if the given
 * message box is full.
 * Equivalently, determine
 * if sending to this mbox
 * would cause a process
 * to block.
 */
int do_mbox_is_full(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
  if(MessageBoxen[mbox].full.value==MAX_MESSAGE_LENGTH)
  return 1;
  else 
  return 0;

}

/* Enqueues a message onto
 * a message box.  If the
 * message box is full, the
 * process will block until
 * it can add the item.
 * You may assume that the
 * message box ID has been
 * properly opened before this
 * call.
 * The message is 'nbytes' bytes
 * starting at offset 'msg'
 */
void do_mbox_send(mbox_t mbox, void *msg, int nbytes)
{
  (void)mbox;
  (void)msg;
  (void)nbytes;
  /* TODO */
 // enter_critical();
  //lock_acquire(&MessageBoxen[mbox].mutex);
  semaphore_down(&MessageBoxen[mbox].empty);
  bcopy(msg,&MessageBoxen[mbox].messages[MessageBoxen[mbox].send],nbytes*sizeof(char));
 // printf(12,1,"in mbox %d send send message is %d",mbox,(*(int *)msg));
 // printf(13,1,"in mbox %d send message copy is %d",mbox,MessageBoxen[mbox].messages[MessageBoxen[mbox].send]);
  MessageBoxen[mbox].send = (MessageBoxen[mbox].send+ 1)%MAX_MESSAGE_LENGTH;
  semaphore_up(&MessageBoxen[mbox].full);
  //lock_release(&MessageBoxen[mbox].mutex);
 // leave_critical();
}

/* Receives a message from the
 * specified message box.  If
 * empty, the process will block
 * until it can remove an item.
 * You may assume that the
 * message box has been properly
 * opened before this call.
 * The message is copied into
 * 'msg'.  No more than
 * 'nbytes' bytes will by copied
 * into this buffer; longer
 * messages will be truncated.
 */
void do_mbox_recv(mbox_t mbox, void *msg, int nbytes)
{
  (void)mbox;
  (void)msg;
  (void)nbytes;
  /* TODO */
 // enter_critical();
  //printf(2,1," into do_mbox_recv,mbox:%d",mbox);
  //lock_acquire(&MessageBoxen[mbox].mutex);
  semaphore_down(&MessageBoxen[mbox].full);
  //printf(13,1,"out semaphore down");
  bcopy(&MessageBoxen[mbox].messages[MessageBoxen[mbox].receive],msg,nbytes*sizeof(char));
 // printf(14,1,"in mbox %d recv recve message is %d",mbox,(*(int *)msg));
 // printf(15,1,"in mbox %d recv message copy is %d",mbox,MessageBoxen[mbox].messages[MessageBoxen[mbox].send]);
  MessageBoxen[mbox].receive = (MessageBoxen[mbox].receive+ 1)%MAX_MESSAGE_LENGTH;
  semaphore_up(&MessageBoxen[mbox].empty);
  //lock_release(&MessageBoxen[mbox].mutex);
 // leave_critical();
}


