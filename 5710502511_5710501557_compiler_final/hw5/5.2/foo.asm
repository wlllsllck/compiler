.data
a: .word 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
newline : .asciiz "\n"

.text
.globl main

main:
    addi $a0, $0, 4             # input j
    addi $sp, $sp, -4
    sw $a0, 0($sp)              # push j
    addi $a0, $0, 5             # input i
    addi $sp, $sp, -4
    sw $a0, 0($sp)              # push i
    jal foo                     # call foo(i, j)
    addi $sp, $sp, 8
    li $v0, 10
    syscall

foo:
    addi $sp, $sp, -8
    move $fp, $sp
    sw $fp, 0($fp)
    sw $ra, 4($fp)
    addi $a0, $0, 2
    addi $sp, $sp, -4
    sw $a0, 0($sp)
    lw $a0, 8($fp)              # load i
    sll $a0, $a0, 2
    la $t0, a
    add $a0, $t0, $a0
    lw $a0, 0($a0)              # $a0 = a[i]
    bgt $a0, $0, TRUE_STATE

    FALSE_STATE:
        lw $a0, 12($fp)         # load j
        sll $a0, $a0, 2
        add $a0, $t0, $a0
        lw $a0, 0($a0)
        sw $a0, 0($sp)          # push y = a[j]
        j EXIT_STATE

    TRUE_STATE:
        lw $a0, 12($fp)         # load j
        sll $a0, $a0, 2
        add $a0, $t0, $a0
        lw $t1, 0($sp)
        sw $t1, 0($a0)

    EXIT_STATE:
        lw $a0, 8($fp)          # load i
        sll $a0, $a0, 2
        add $a0, $t0, $a0
        lw $a0, 0($a0)
        lw $t1, 0($sp)
        multu $a0, $t1          # a[i] * y
        mflo $a0
        li $v0, 1
        syscall
        li $v0, 4
        la $a0, newline
        syscall
        lw $ra, 4($fp)
        lw $fp, 0($fp)
        addi $sp, $sp, 12
        jr $ra

