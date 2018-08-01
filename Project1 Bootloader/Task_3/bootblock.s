	.text
	.globl main

main:
	# check the offset of main
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	_start:
	li $4, 0xa0800200
	li $5, 0x200
	li $6, 0x150

	jal 0x8007b1a8
	nop
	jal 0xa080026c

	#need add code
	#read kernel

