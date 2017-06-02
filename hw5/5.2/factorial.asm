.data
newline: .asciiz "\n"

.text
.globl main

main:
    li $a0, 10                  # input n
    addi $sp, $sp, -4
    sw $a0, 0($sp)
    jal Factorial
    addi $sp, $sp, 4
    li $v0, 1
    syscall
    li $v0, 4
    la $a0, newline
    syscall
    li $v0, 10
    syscall

Factorial:
    addi $sp, $sp, -8
    sw $ra, 4($sp)
    sw $fp, 0($sp)
    move $fp, $sp
    lw $a0, 8($fp)
    beq $a0, $0, TRUE_STATE

    FALSE_STATE:
        addi $sp, $sp, -8
        sw $a0, 4($sp)
        addi $a0, $a0, -1
        sw $a0, 0($sp)
        jal Factorial
        lw $t1, 4($sp)
        mult $a0, $t1
        mflo $a0
        addi $sp, $sp, 8
        j EXIT_STATE

    TRUE_STATE:
        li $a0, 1

    EXIT_STATE:
        lw $ra, 4($fp)
        lw $fp, 0($fp)
        addi $sp, $sp, 8
        jr $ra
