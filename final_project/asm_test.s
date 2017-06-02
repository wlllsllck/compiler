.text
.globl main
nop
main:
addiu $sp $sp -48
sw $ra 44($sp)
sw $fp 40($sp)
movz $ra $ra $0
move $fp $sp
lw $t0 48($sp)
li $t1 2
mult $t0 $t1
mflo $t1
move $t0 $t1
jal $ra
