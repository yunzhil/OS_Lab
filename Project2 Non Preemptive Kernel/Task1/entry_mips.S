#include "regs.h"
	.text
	.align	2
	.globl  get_timer	
	.ent	get_timer
	.type	get_timer, @function
get_timer:
	mfc0 $2,$9
        nop
        j       ra
        nop
	.align	2
	.globl  kernel_entry	
	.ent	kernel_entry
	.type	kernel_entry, @function
kernel_entry:
	addiu sp, sp, -24
	sw ra, 0(sp)
	bnez $4, 1f
	nop

	jal do_yield
	nop
	beqz $0,2f
	nop

1:
	jal do_exit
	nop
2:
	lw ra, 0(sp)
	addiu sp, sp, 24
	jr ra
	nop

	.align	2
	.globl  scheduler_entry	
	.ent	scheduler_entry
	.type	scheduler_entry, @function
scheduler_entry:
	# call scheduler, which will set new current process
	# need student add
	jal scheduler
	la ra, current_running
	lw ra,(ra)
	lw AT,1*4(ra)
	lw v0,2*4(ra)
	lw v1,3*4(ra)
	lw a0,4*4(ra)
	lw a1,5*4(ra)
	lw a2,6*4(ra)
	lw a3,7*4(ra)
	lw t0,8*4(ra)
	lw t1,9*4(ra)
	lw t2,10*4(ra)
	lw t3,11*4(ra)
	lw t4,12*4(ra)
	lw t5,13*4(ra)
	lw t6,14*4(ra)
	lw t7,15*4(ra)
	lw s0,16*4(ra)
	lw s1,17*4(ra)
	lw s2,18*4(ra)
	lw s3,19*4(ra)
	lw s4,20*4(ra)
	lw s5,21*4(ra)
	lw s6,22*4(ra)
	lw s7,23*4(ra)
	lw t8,24*4(ra)
	lw t9,25*4(ra)
	lw k0,26*4(ra)
	lw k1,27*4(ra)
	lw gp,28*4(ra)
	lw sp,29*4(ra)
	lw fp,30*4(ra)
	lw ra,31*4(ra)
	jr ra

	
	.align	2
	.globl  save_pcb	
	.ent	save_pcb
	.type	save_pcb, @function
save_pcb:
	# save the pcb of the currently running process
	# need student add
	la k0, current_running
	lw k0, (k0)
	sw AT,1*4(k0)
	sw v0,2*4(k0)
	sw v1,3*4(k0)
	sw a0,4*4(k0)
	sw a1,5*4(k0)
	sw a2,6*4(k0)
	sw a3,7*4(k0)
	sw t0,8*4(k0)
	sw t1,9*4(k0)
	sw t2,10*4(k0)
	sw t3,11*4(k0)
	sw t4,12*4(k0)
	sw t5,13*4(k0)
	sw t6,14*4(k0)
	sw t7,15*4(k0)
	sw s0,16*4(k0)
	sw s1,17*4(k0)
	sw s2,18*4(k0)
	sw s3,19*4(k0)
	sw s4,20*4(k0)
	sw s5,21*4(k0)
	sw s6,22*4(k0)
	sw s7,23*4(k0)
	sw t8,24*4(k0)
	sw t9,25*4(k0)
	sw k0,26*4(k0)
	sw k1,27*4(k0)
	sw gp,28*4(k0)
# miss sp
	sw fp,30*4(k0)
# miss ra
	lw k1,20(sp)
	sw k1,31*4(k0)
	addiu sp,24
	sw sp, 29*4(k0)
#	addiu sp,-24
	jr ra
