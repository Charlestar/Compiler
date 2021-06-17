.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text

read:
  li $v0, 4
  la $a0, _prompt
  syscall
  li $v0, 5
  syscall
  jr $ra

write:
  li $v0, 1
  syscall
  li $v0, 4
  la $a0, _ret
  syscall
  move $v0, $0
  jr $ra

fact:
  li $8, 1
  beq $4, $8, L0
  j L1
L0:
  move $v0, $4
  jr $ra
  j L2
L1:
  addi $9, $4, -1
  move $a0, $9
  sw $8, -4($sp)
  sw $9, -8($sp)
  sw $10, -12($sp)
  sw $11, -16($sp)
  sw $12, -20($sp)
  sw $13, -24($sp)
  sw $14, -28($sp)
  sw $15, -32($sp)
  sw $16, -36($sp)
  sw $17, -40($sp)
  sw $18, -44($sp)
  sw $19, -48($sp)
  sw $20, -52($sp)
  sw $21, -56($sp)
  sw $22, -60($sp)
  sw $23, -64($sp)
  sw $24, -68($sp)
  sw $25, -72($sp)
  addi $sp, $sp, -72
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $25, 4($sp)
  lw $24, 8($sp)
  lw $23, 12($sp)
  lw $22, 16($sp)
  lw $21, 20($sp)
  lw $20, 24($sp)
  lw $19, 28($sp)
  lw $18, 32($sp)
  lw $17, 36($sp)
  lw $16, 40($sp)
  lw $15, 44($sp)
  lw $14, 48($sp)
  lw $13, 52($sp)
  lw $12, 56($sp)
  lw $11, 60($sp)
  lw $10, 64($sp)
  lw $9, 68($sp)
  lw $8, 72($sp)
  addi $sp, $sp, 72
  move $10, $v0
  mul $8, $4, $10
  move $v0, $8
  jr $ra
L2:

main:
  addi $sp, $sp, -4
  sw $9, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $9, $v0
  addi $sp, $sp, -4
  sw $10, 0($sp)
  move $10, $9
  li $11, 1
  bgt $10, $11, L3
  j L4
L3:
  move $a0, $10
  sw $8, -4($sp)
  sw $9, -8($sp)
  sw $10, -12($sp)
  sw $11, -16($sp)
  sw $12, -20($sp)
  sw $13, -24($sp)
  sw $14, -28($sp)
  sw $15, -32($sp)
  sw $16, -36($sp)
  sw $17, -40($sp)
  sw $18, -44($sp)
  sw $19, -48($sp)
  sw $20, -52($sp)
  sw $21, -56($sp)
  sw $22, -60($sp)
  sw $23, -64($sp)
  sw $24, -68($sp)
  sw $25, -72($sp)
  addi $sp, $sp, -72
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $25, 4($sp)
  lw $24, 8($sp)
  lw $23, 12($sp)
  lw $22, 16($sp)
  lw $21, 20($sp)
  lw $20, 24($sp)
  lw $19, 28($sp)
  lw $18, 32($sp)
  lw $17, 36($sp)
  lw $16, 40($sp)
  lw $15, 44($sp)
  lw $14, 48($sp)
  lw $13, 52($sp)
  lw $12, 56($sp)
  lw $11, 60($sp)
  lw $10, 64($sp)
  lw $9, 68($sp)
  lw $8, 72($sp)
  addi $sp, $sp, 72
  move $12, $v0
  addi $sp, $sp, -4
  sw $8, 0($sp)
  move $8, $12
  j L5
L4:
  li $8, 1
L5:
  move $a0, $8
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  sw $9, 0($sp)
  li $9, 0
  move $v0, $9
  jr $ra
