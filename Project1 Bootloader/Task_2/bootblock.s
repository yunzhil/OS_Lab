.data
output: .ascii "Welcome to OS\n"


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


begin:
    la $8, output
    addiu $9, $8, 14
    li $10, 0xbfe48000

print:
    lb $11,0($8)
    sb $11,0($10)
    addiu $8, $8, 1
    beq $8, $9, end
    li $11, 512

loop:
    addiu $11, $11, -1
    bnez $11, loop
    j print

end:
    j end
