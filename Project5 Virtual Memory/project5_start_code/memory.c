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

//#define CLOCK // 表针算法

#define MEM_START 0xa0908000
#define SWAP_START 0xa0a08000

#define SWAPABLE_PAGES 256

uint32_t page_vaddr( int i );
uint32_t page_paddr( int i );
uint32_t swap_vaddr( int i );
uint32_t swap_paddr( int i );
uint32_t va2pa( uint32_t va );
uint32_t pa2va( uint32_t pa );
void set_Read( uint32_t pa );

/* Static global variables */
// Keep track of all pages: their vaddr, status, and other properties
static page_map_entry_t page_map[ PAGEABLE_PAGES ];

uint32_t page_map_queue[PAGEABLE_PAGES];
uint32_t ptr_page, ptr_swap;

// other global variables...
/*static lock_t page_fault_lock;*/

/* TODO: Returns physical address of page number i */
uint32_t page_vaddr( int i ) {
    return MEM_START + i * PAGE_SIZE;
}

/* TODO: Returns virtual address (in kernel) of page number i */
uint32_t page_paddr( int i ) {
    return va2pa(page_vaddr(i));
}

uint32_t swap_vaddr( int i ) {
    return SWAP_START + i * PAGE_SIZE;
}

uint32_t swap_paddr( int i ) {
    return va2pa(swap_vaddr(i));
}

/* get the physical address from virtual address (in kernel) */
uint32_t va2pa( uint32_t va ) {
    return (uint32_t) va - 0xa0000000;
}

/* get the virtual address (in kernel) from physical address */
uint32_t pa2va( uint32_t pa ) {
    return (uint32_t) pa + 0xa0000000;
}

void set_Read( uint32_t pte ) {
    int index = (pa2va((pte & 0xffffffc0) << 6) - MEM_START) / PAGE_SIZE;
    page_map[index].Read = 1;
}

void pageswap_init() {
    ptr_page = ptr_swap = 0;
    int i;
    for(i=0;i<PAGEABLE_PAGES;i++) {
        page_map_queue[i] = i;
    }
}

uint32_t get_swap(int CLOCK) {
    ptr_page %= PAGEABLE_PAGES;
if(CLOCK==1)
    while(page_map[ptr_page].status == PM_PINNED || page_map[ptr_page].Read) {
        if(page_map[ptr_page].Read) page_map[ptr_page].Read = 0;
        ptr_page = (ptr_page + 1) % PAGEABLE_PAGES;
    }
else
    while(page_map[ptr_page].status == PM_PINNED) {
        if(page_map[ptr_page].Read) page_map[ptr_page].Read = 0;
        ptr_page = (ptr_page + 1) % PAGEABLE_PAGES;
    }
    return ptr_page++;
}

int swap_alloc() {
    ptr_swap %= SWAPABLE_PAGES;
    ASSERT2(ptr_swap < SWAPABLE_PAGES, "Swap Full !");
    return ptr_swap++;
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
int page_alloc( int pinned, uint32_t page_ptr ) {
    int free_index = PAGEABLE_PAGES, i;
    free_index = get_swap(1);
    ASSERT( free_index < PAGEABLE_PAGES );

    if(page_map[free_index].status != PM_FREE) {
        int swap_index = swap_alloc();
        *(uint32_t*)(page_map[free_index].page_ptr) = (1 << 31) | (swap_index << 6);
        tlb_flush2();
        bcopy(page_vaddr(free_index), swap_vaddr(swap_index), PAGE_SIZE);
    }
    if(pinned) {
        page_map[free_index].status = PM_PINNED;
        page_map[free_index].page_ptr = 0;
    } else {
        page_map[free_index].status = PM_ALLOCATED;
        page_map[free_index].page_ptr = page_ptr;
    }
    page_map[free_index].Read = 0;
    return free_index;
}

/* TODO:
 * This method is only called once by _start() in kernel.c
 */
uint32_t init_memory( void ) {

    // initialize all pageable pages to a default state
    int i;
    for(i=0;i<PAGEABLE_PAGES;i++) {
        page_map[i].status = PM_FREE;
        page_map[i].page_ptr = 0;
        page_map[i].Read = 0;
    }

    pageswap_init();
}

/* TODO:
 *
 */
uint32_t setup_page_table( int pid ) {
    uint32_t page_table;

    // alloc page for page table
    int free_index = page_alloc(1, 0);
    page_table = page_vaddr(free_index);

    // initialize PTE and insert several entries into page tables using insert_page_table_entry
    bzero(page_table, PAGE_SIZE);
    return page_table;
}

uint32_t do_tlb_miss(uint32_t vaddr, int pid) {
    return 0;
}

void create_pte(uint32_t vaddr, int pid) {
    return;
}
