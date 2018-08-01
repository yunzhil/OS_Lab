#include "asm.h"
#include "regs.h"
#include "cp0regdefs.h"

  /* Hardcoded values. Do not change these!! */
  /* Offset of the kernel context in a PCB */
  .equ  KERNEL, 8
  /* Offset of the user context in a PCB */
  .equ  USER, 164
  /* Offset of %esp in a context */
  .equ  STACK, TF_REG29
  /* Offset of field nested_count in a PCB */
  .equ  NESTED_COUNT, 320

.macro PUTCHAR ch label
	  addi  k0,$0,10000
	\label:
	addi  k0,k0,-1
	bne k0,$0,\label

	addi  k0,$0,\ch
	la    k1,0xbfe48000
	sb    k0,0(k1)
.endm

.macro STI
  mfc0    k0, CP0_STATUS
  li      k1, (STATUS_CU0 | 0x1)
  or      k0, k1
  mtc0    k0, CP0_STATUS
.endm

.macro CLI
  mfc0    k0, CP0_STATUS
  li      k1, (STATUS_CU0 | 0x1)
  or      k0, k1
  xori    k0, 0x1
  mtc0    k0, CP0_STATUS
.endm

/* Do not change any of these macros! */
/* Save registers/flags to the specified offset in the current PCB */
.macro SAVE_CONTEXT offset
/* TODO: need add */
lw      $26, current_running
  addi    $26, $26, \offset

  sw      $1, 4(k0)
  sw      $2, 8(k0)
  sw      $3, 12(k0)
  sw      $4, 16(k0)
  sw      $5, 20(k0)
  sw      $6, 24(k0)
  sw      $7, 28(k0)
  sw      $8, 32(k0)
  sw      $9, 36(k0)
  sw      $10, 40(k0)
  sw      $11, 44(k0)
  sw      $12, 48(k0)
  sw      $13, 52(k0)
  sw      $14, 56(k0)
  sw      $15, 60(k0)
  sw      $16, 64(k0)
  sw      $17, 68(k0)
  sw      $18, 72(k0)
  sw      $19, 76(k0)
  sw      $20, 80(k0)
  sw      $21, 84(k0)
  sw      $22, 88(k0)
  sw      $23, 92(k0)
  sw      $24, 96(k0)
  sw      $25, 100(k0)
  sw      $26, 104(k0) 
  sw      $27, 108(k0) 
  sw      $28, 112(k0)
  sw      $29, 116(k0)
  sw      $30, 120(k0)
  sw      $31, 124(k0)

  mfc0    $27, CP0_STATUS
  nop
  sw      $27, 128(k0)
  mfc0    $27, CP0_TAGHI
  nop
  sw      $27, 132(k0)
  mfc0    $27, CP0_TAGLO
  nop
  sw      $27, 136(k0)
  mfc0    $27, CP0_BADVADDR
  nop
  sw      $27, 140(k0)
  mfc0    $27, CP0_CAUSE
  nop
  sw      $27, 144(k0)
  mfc0    $27, CP0_EPC
  nop
  sw      $27, 148(k0)
/* TODO: end */
.endm

/* Restore registers/flags from the specified offset in the current PCB */
.macro RESTORE_CONTEXT offset
/* TODO: need add */
 lw      $26, current_running
  addi    $26, $26, \offset

  lw      $1, 4(k0)
  lw      $2, 8(k0)
  lw      $3, 12(k0)
  lw      $4, 16(k0)
  lw      $5, 20(k0)
  lw      $6, 24(k0)
  lw      $7, 28(k0)
  lw      $8, 32(k0)
  lw      $9, 36(k0)
  lw      $10, 40(k0)
  lw      $11, 44(k0)
  lw      $12, 48(k0)
  lw      $13, 52(k0)
  lw      $14, 56(k0)
  lw      $15, 60(k0)
  lw      $16, 64(k0)
  lw      $17, 68(k0)
  lw      $18, 72(k0)
  lw      $19, 76(k0)
  lw      $20, 80(k0)
  lw      $21, 84(k0)
  lw      $22, 88(k0)
  lw      $23, 92(k0)
  lw      $24, 96(k0)
  lw      $25, 100(k0)
  /* lw      k0, 104(k0) */
  /* lw      k1, 108(k0) */
  lw      $28, 112(k0)
  lw      $29, 116(k0)
  lw      $30, 120(k0)
  lw      $31, 124(k0)

  lw      k1, 128(k0)
  mtc0    k1, CP0_STATUS
  nop
  lw      k1, 132(k0)
  mtc0    k1, CP0_TAGHI
  nop
  lw      k1, 136(k0)
  mtc0    k1, CP0_TAGLO
  nop
  lw      k1, 140(k0)
  mtc0    k1, CP0_BADVADDR
  nop
  lw      k1, 144(k0)
  mtc0    k1, CP0_CAUSE
  nop
  lw      k1, 148(k0)
  mtc0    k1, CP0_EPC
  nop
/* TODO: end */
.endm

/* Increment disable_count, panicking on a double ENTER
 * Interrupts should be disabled before this macro is used
 */
.macro ENTER_CRITICAL
  la      k0, disable_count
  lw      k1, 0(k0)
  bne     k1, $0, panice
  nop
  addi    k1, k1, 1
  sw      k1, 0(k0)
.endm

/* Decrement disable_count, panicking on a double LEAVE
 * Interrupts should be enabled after this macro is used
 */
.macro LEAVE_CRITICAL
  la      k0, disable_count
  lw      k1, 0(k0)
  addi    k1, k1, -1
  bne     k1, $0, panicl
  nop
  sw      k1, 0(k0)
.endm

/* TODO: Test current_running->nested_count, clobbering only eflags */
.macro TEST_NESTED_COUNT
lw  k1,current_running
lw  k1, NESTED_COUNT(k1)
.endm

/** TODO end */

  .data
// Temporary storage for 32-bit registers
scratch:
  .long    0
// Temporary storage for 32-bit registers
scratch2:
  .long    0
// Temporary storage for syscall return value
return_value:
  .long    0
// Signals status of interrupts
    .globl    disable_count
disable_count:
  .long    0

  .text

NESTED(scheduler_entry,0,ra)
	/* TODO: need add */
  SAVE_CONTEXT(KERNEL)
  jal     scheduler
  nop
  RESTORE_CONTEXT(KERNEL)
  jr      ra
  nop
	/* TODO: end */
END(scheduler_entry)

LEAF(asm_start)
  /* Disable interrupts */
  mtc0    zero, CP0_STATUS

  /* Disable watch exception. */
  mtc0    zero, CP0_WATCHLO
  mtc0    zero, CP0_WATCHHI

  /* disable kernel mode cache */
  mfc0    t0, CP0_CONFIG
  and     t0, ~0x7
  ori     t0, 0x2
  mtc0    t0, CP0_CONFIG

  jr  ra
END(asm_start)

LEAF(reset_timer)
  mtc0 $0, CP0_COUNT
  mtc0 a0, CP0_COMPARE
  jr ra
END(reset_timer)

  .global exception_handler_start
  .global exception_handler_end
NESTED(general_exception_handler,0,sp)
exception_handler_start:
  .set  noat
  .set  mips32

  CLI

  mfc0  k0, CP0_CAUSE
  andi  k0, k0, CAUSE_EXCCODE   /* k0 = {exc_code,00} */
  la    k1, interrupt_handlers
  add   k0, k0, k1
  lw    k0, 0(k0)
  jr  k0     /* interrupt_handlers[exc_code](); */
exception_handler_end:
END(general_exception_handler)
  .set at
  .set reorder
  .text

LEAF(return_from_exception)
	STI

	eret
END(return_from_exception)

NESTED(simple_handler,0,sp)
  SAVE_CONTEXT(KERNEL)

  mfc0  k0, CP0_CAUSE           /* Read Cause register for IP bits */
  //mfc0  k1, CP0_STATUS        /* and Status register for IM bits */
  andi  k0, k0, CAUSE_IPL       /* Keep only IP bits from Cause */
  //and   k0, k0, k1            /* and mask with IM bits */
  clz   k0, k0                  /* Find first bit set, IP7..IP0; k0 = 16..23 */
  xori  k0, k0, 0x17            /* 16..23 => 7..0 */
  add   a0, k0, $0

  mfc0  k0, CP0_CAUSE
	andi  k0, k0, CAUSE_EXCCODE   /* k0 = {exc_code,00} */
  srl   k0, k0, 2
  add   a1, k0, $0

  jal   c_simple_handler

  mfc0  k0, CP0_CAUSE           /* Read Cause register for IP bits */
	andi  k1, k0, CAUSE_IPL       /* Keep only IP bits from Cause */
	xor   k0, k0, k1
  mtc0  k0, CP0_CAUSE

  LEAVE_CRITICAL
  RESTORE_CONTEXT(KERNEL)
  j return_from_exception
END(simple_handler)

NESTED(handle_syscall,0,sp)
  SAVE_CONTEXT(USER)
  //RESTORE_CONTEXT(KERNEL)
  add   a3,a2,$0
  add   a2,a1,$0
  add   a1,a0,$0
  add   a0,v0,$0

  jal system_call_helper

  LEAVE_CRITICAL
  RESTORE_CONTEXT(USER)
  j return_from_exception
  nop
END(handle_syscall)

NESTED(handle_int,0,sp)
  /* TODO: timer_irq */
  /* read int IP and handle clock interrupt or just call do_nothing */
  SAVE_CONTEXT(USER)

  mfc0  k0, CP0_CAUSE
  nop
  andi  k0, k0, CAUSE_IPL
  clz   k0, k0
  xori  k0, k0, 0x17

  addiu k1, zero, 7
  beq   k0, k1, time_irq
  nop

  jal   clear_int
  nop

int_done:
  RESTORE_CONTEXT(USER)
  j     return_from_exception
  nop

#define TIMER_INTERVAL 150000000

time_irq:
  li    a0, TIMER_INTERVAL
  jal   reset_timer

  jal   clear_int

  lw    k1, time_elapsed
  addiu k1, k1, 1000
  sw    k1, time_elapsed
  TEST_NESTED_COUNT
  bne   k1, zero, int_done
  nop

  ENTER_CRITICAL
  lw    k0, current_running
  li    k1, 1
  sw    k1, NESTED_COUNT(k0)

  jal   put_current_running
  nop
  jal   scheduler_entry
  nop

  lw    k0, current_running
  sw    zero, NESTED_COUNT(k0)


  LEAVE_CRITICAL
  j     int_done
  nop

clear_int:
  mfc0  k0, CP0_CAUSE
  nop
  andi  k1, k0, CAUSE_IPL
  xor   k0, k0, k1
  mtc0  k0, CP0_CAUSE
  nop
  jr ra
  nop
  /* TODO:end */
END(handle_int)


LEAF(begin_here)
  LEAVE_CRITICAL
  j     int_done
END(begin_here)


LEAF(get_cp0_status)
  mfc0  v0, CP0_STATUS
  jr ra
END(get_cp0_status)

LEAF(set_cp0_status)
  mtc0  a0, CP0_STATUS
  jr ra
END(set_cp0_status)


LEAF(enter_critical)
  CLI
  ENTER_CRITICAL
  jr ra
END(enter_critical)

LEAF(leave_critical)
  LEAVE_CRITICAL
  STI
  jr ra
END(leave_critical)

/* Error Message: Write "PANIC" to the screen and enter an infinite loop */
LEAF(panic)
  CLI
  PUTCHAR 0x50 P1// 'P'
  j panic
END(panic)
LEAF(panice)
  CLI
  PUTCHAR 69 P2// 'P'
  j panic
END(panice)
	LEAF(panicl)
	  CLI
	  PUTCHAR 76 P3// 'P'
	  j panic
	END(panicl)
//  cli
//  movl    $0xb800a, %esp
//  pushw    $0x4f43
//  pushw    $0x4f49
//  pushw    $0x4f4e
//  pushw    $0x4f41
//  pushw    $0x4f50
LEAF(infinite_loop)
//        hlt
  j    infinite_loop
END(infinite_loop)
