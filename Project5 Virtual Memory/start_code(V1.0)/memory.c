/* Author(s): <Your name here>
 * Implementation of the memory manager for the kernel.
 */

#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "memory.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"

#define MEM_START 0xa0908000
static int page_fault_num;
/* Static global variables */
// Keep track of all pages: their vaddr, status, and other properties
static page_map_entry_t page_map[ PAGEABLE_PAGES ];

// other global variables...
/*static lock_t page_fault_lock;*/

/* TODO: Returns physical address of page number i */
uint32_t page_vaddr( int i ) {
    return MEM_START + PAGE_SIZE * i;
}

/* TODO: Returns virtual address (in kernel) of page number i */
uint32_t page_paddr( int i ) {
    return MEM_START + PAGE_SIZE * i - 0xa0000000;
}

/* get the physical address from virtual address (in kernel) */
uint32_t va2pa( uint32_t va ) {
    return (uint32_t) va - 0xa0000000;
}

/* get the virtual address (in kernel) from physical address */
uint32_t pa2va( uint32_t pa ) {
    return (uint32_t) pa + 0xa0000000;
}


// TODO: insert page table entry to page table
void insert_page_table_entry( uint32_t *table, uint32_t vaddr, uint32_t paddr,
                              uint32_t flag, uint32_t pid ) {
    // insert entry

    // tlb flush
}

/* TODO: Allocate a page. Return page index in the page_map directory.
 *
 * Marks page as pinned if pinned == TRUE.
 * Swap out a page if no space is available.
 */
int page_alloc( int pinned ,int pid) {
 
    // code here
    int i=0;
    for(i=0;i<PAGEABLE_PAGES;i++)
    {
        if(!page_map[i].busy)
        {
            page_map[i].busy=1;
            page_map[i].pinned = pinned;
            page_map[i].pid=pid;
            ASSERT( i < PAGEABLE_PAGES );
//            printf(3,1,"set up page table index is %d,pid is %d",i,pid);
            return i;
        }
    }
    if(i==PAGEABLE_PAGES)
    {
        //means we have a page change
        for(i=0;i<PAGEABLE_PAGES;i++)
        {
            if(!page_map[i].pinned)
            {
               // printf(4,1,"use old page table index is %d,pid is %d",i,pid);
                int father = page_map[i].pid;
                page_table_entry_t* page_table = pcb[father].page_table;
              //  printf(5,1,"father is %d",father);
                int entry_num = (pcb[father].size + 0xfff)/0x1000;
                int j=0;
                for(j=0;j<entry_num;j++)
                {
                   // printf(11,1,"father is %d,j 's pfn_flags is %x",father,page_table[j].PFN_FLAGS);
                   // printf(12,1,"page_paddr is %x",(page_paddr(i)&0xfffff000)>>6);
                    if(((page_paddr(i)&0xfffff000)>>6|0x00000016)==(page_table[j].PFN_FLAGS))
                    {
//                        printf(10,1,"found one,going to valid,father is %d",father);
                        page_table[j].PFN_FLAGS = page_table[j].PFN_FLAGS&0xfffffffd;
                      //  tlb_flush((page_table[j].VPN&0xfffff000)|father);
                        //tlb_flush_all();
                       // bcopy((void*)page_vaddr(i),(void*)(pcb[father].loc+j*PAGE_SIZE),PAGE_SIZE);
                        ASSERT2(j == 0, "j is not zero");     
                        break;
                    }

                }
                page_map[i].pid = pid;
                return i;
                //invalid father's page map done
                //do tlb_flush
                //refill this physical pagg
            }
        }
    }
}

/* TODO: 
 * This method is only called once by _start() in kernel.c
 */
uint32_t init_memory( void ) {
    
    // initialize all pageable pages to a default state
    page_fault_num=0;
    int i=0;
    for(i=0;i<PAGEABLE_PAGES;i++)
    {
        page_map[i].busy=0;
        page_map[i].dirty=0;
        page_map[i].pinned=0;
        page_map[i].paddr = va2pa(MEM_START+i*PAGE_SIZE);
    }
    return 0;

}

/* TODO: 
 * 
 */
uint32_t setup_page_table( int pid ) {
   //uint32_t page_table;

    // alloc page for page table 
    int page_map_num=0;
    page_map_num = page_alloc(1,pid);
   // printf(1,1,"alloc page table  %d,pid is %d",page_map_num,pid);
    page_table_entry_t* page_table = (page_table_entry_t*)page_vaddr(page_map_num);

    int entry_num = (pcb[pid].size + 0xfff)/0x1000;
    int i=0;
   // printf(5,1,"entry_num is %d, pid is %d",entry_num,pid);
    for(i=0;i<entry_num;i++)
    {
      //  int index = page_alloc(0);
     //   printf(2,1,"set up page table index is %d,pid is %d",index,pid);
      //  int src = pcb[pid].loc+PAGE_SIZE*i;
      //  int dest = page_vaddr(index);
      //  bcopy((void*)src,(void*)dest,PAGE_SIZE);
      //  int paddr = page_paddr(index);
        page_table[i].VPN=pcb[pid].entry_point+PAGE_SIZE*i;
     //   printf(3,1,"now the entry point is %d,pid is %d",pcb[pid].entry_point,pid);
       // page_table[i].PFN_FLAGS = ((0xfffff000)>>6)|0x00000004;
        page_table[i].PFN_FLAGS = ((0xfffff000)>>6);
        tlb_flush((page_table[i].VPN&0xfffff000)|pid);
        //tlb_flush_all();
    }
    // initialize PTE and insert several entries into page tables using insert_page_table_entry
    return page_table;
}


uint32_t do_tlb_miss(uint32_t vaddr) {
    int i=0;
    page_table_entry_t *page_table = current_running->page_table;
    //printf(1,1,"one tlb miss, pid is %d, page table address is %x",current_running->pid,page_table);
   // int entry_num = (current_running->size + 0xfff)/0x1000;
    for(i=0;i<PAGE_N_ENTRIES;i++)
    {
        if((page_table[i].VPN&0xfffff000)==(vaddr&0xfffff000))
        {
           // tlb_flush_all();
           // tlb_flush_all();
           // tlb_flush_all();
           // tlb_flush((page_table[i].VPN&0xfffff000)|current_running->pid);
            //MTHI((vaddr&0xfffff000)|current_running->pid);
           // MTHI((vaddr&0xffffe000)|current_running->pid);
            if(vaddr&0x1000)
            {
               // MTLO1((page_table[i].PFN_FLAGS&0x7fffffff));
               if((page_table[i].PFN_FLAGS)&0x2)
                {
                    tlb_flush(page_table[i].VPN&0xfffff000);
                     printf(5,1,"hit without change pagem pid is %d ",current_running->pid);
                   MTLO1((page_table[i].PFN_FLAGS));
                    MTLO0((page_table[i].PFN_FLAGS));
                }
                else
                {
                  //  page_fault();
                   page_fault_num++;
                   printf(6,1,"a page fault current_running pid is %d, total page_fault num is %d",current_running->pid,page_fault_num);
                   int new=page_alloc(0,current_running->pid);
                   int pid = current_running->pid;
                   int index = (vaddr-pcb[pid].entry_point)/0x1000;
                   ASSERT2(index==0,"index is not zero");
                   printf(7,1,"index is %d, pid is %d",index,pid);
                   int src = pcb[pid].loc+PAGE_SIZE*index;
                   int dest = page_vaddr(new);
                   //printf(8,1,"source is %x, dest is %x, pid is %d",src,dest,pid);
                  // tlb_flush((page_table[i].VPN&0xfffff000)|pid);
                   bcopy((void*)src,(void*)dest,PAGE_SIZE);
                   int paddr = page_paddr(new);
                   page_table[i].VPN=pcb[pid].entry_point+PAGE_SIZE*index;
     //   printf(3,1,"now the entry point is %d,pid is %d",pcb[pid].entry_point,pid);
                   page_table[i].PFN_FLAGS = ((paddr&0xfffff000)>>6)|0x00000016;
                   //tlb_flush((page_table[i].VPN&0xffffe000)|pid);
                  // printf(8,1,"source is %x, dest is %x, pid is %d,paddr is %x",src,dest,pid,paddr);
                  // printf(9,1,"page_table[i].VPN is %x,page_table[i].PFN_FLAGS is %x",page_table[i].VPN,page_table[i].PFN_FLAGS);
                   MTLO1((page_table[i].PFN_FLAGS));
                   MTLO0((page_table[i].PFN_FLAGS));
                  // asm volatile("tlbwr");
                }
                //page_fault
            }
            else
            {
                if((page_table[i].PFN_FLAGS)&0x2)
                {
                    MTLO0((page_table[i].PFN_FLAGS));
                    MTLO1((page_table[i].PFN_FLAGS));
                    printf(5,1,"hit without change pagem pid is %d,pfn is %x ",current_running->pid,(page_table[i].PFN_FLAGS));
               }
                else
                {
                   // page_fault();
                   page_fault_num++;
                   printf(6,1,"a page fault current_running pid is %d, total page_fault num is %d",current_running->pid,page_fault_num);
                   int new =page_alloc(0,current_running->pid);
                   //int new=page_alloc(0,current_running->pid);
                   int pid = current_running->pid;
                   int index = (vaddr-pcb[pid].entry_point)/0x1000;
                   ASSERT2(index==0,"index is not zero");
//                   printf(6,1,"index is %d, pid issss %d",index,pid);
                   int src = pcb[pid].loc+PAGE_SIZE*index;
                   int dest = page_vaddr(new);
                //   printf(8,1,"source is %x, dest is %x, pid is %d",src,dest,pid);
                   //tlb_flush((page_table[i].VPN&0xfffff000)|pid);
                   bcopy((void*)src,(void*)dest,PAGE_SIZE);
                   int paddr = page_paddr(new);
                   page_table[i].VPN=pcb[pid].entry_point+PAGE_SIZE*index;
     //   printf(3,1,"now the entry point is %d,pid is %d",pcb[pid].entry_point,pid);
                   page_table[i].PFN_FLAGS = ((paddr&0xfffff000)>>6)|0x00000016;
                   //tlb_flush((page_table[i].VPN&0xffffe000)|pid);
                   printf(8,1,"source is %x, dest is %x, pid is %d,paddr is %x",src,dest,pid,paddr);
                   printf(9,1,"page_table[i].VPN is %x,page_table[i].PFN_FLAGS is %x",page_table[i].VPN,page_table[i].PFN_FLAGS);
                   MTLO0((page_table[i].PFN_FLAGS));
                   MTLO1((page_table[i].PFN_FLAGS));
                 //  asm volatile("tlbwr");

                   printf(10,1,"page fault done, pid is %d",pid);
                }
            }
            return 0;
        }
    }
    return 0;
}

void create_pte(uint32_t vaddr, int pid) {
    return;
}
