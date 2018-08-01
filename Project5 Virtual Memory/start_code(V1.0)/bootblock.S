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
	li $4, 0xa0800200
	li $5, 0x200
L2:
	li $6, 0xc000
	jal 0x8007b1a8
	nop
	jal 0xa0800200
L1:
	nop
	j L1

	#need add code
	#read kernel

