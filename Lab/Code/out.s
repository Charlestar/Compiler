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
  addi $8, $4, -1
  addi $sp, $sp, -16
  sw $4, 12($sp)
  sw $5, 8($sp)
  sw $6, 4($sp)
  sw $7, 0($sp)
  move $a0, $8
  addi $sp, $sp, -72
  sw $8, 68($sp)
  sw $9, 64($sp)
  sw $10, 60($sp)
  sw $11, 56($sp)
  sw $12, 52($sp)
  sw $13, 48($sp)
  sw $14, 44($sp)
  sw $15, 40($sp)
  sw $16, 36($sp)
  sw $17, 32($sp)
  sw $18, 28($sp)
  sw $19, 24($sp)
  sw $20, 20($sp)
  sw $21, 16($sp)
  sw $22, 12($sp)
  sw $23, 8($sp)
  sw $24, 4($sp)
  sw $25, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  jal fact
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $25, 0($sp)
  lw $24, 4($sp)
  lw $23, 8($sp)
  lw $22, 12($sp)
  lw $21, 16($sp)
  lw $20, 20($sp)
  lw $19, 24($sp)
  lw $18, 28($sp)
  lw $17, 32($sp)
  lw $16, 36($sp)
  lw $15, 40($sp)
  lw $14, 44($sp)
  lw $13, 48($sp)
  lw $12, 52($sp)
  lw $11, 56($sp)
  lw $10, 60($sp)
  lw $9, 64($sp)
  lw $8, 68($sp)
  addi $sp, $sp, 72
  addi $sp, $sp, 0
  lw $7, 0($sp)
  lw $6, 4($sp)
  lw $5, 8($sp)
  lw $4, 12($sp)
  addi $sp, $sp, 16
  move $9, $v0
  mul $8, $4, $9
  move $v0, $8
  jr $ra
L2:

main:
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $8, $v0
  move $9, $8
  li $10, 1
  bgt $9, $10, L3
  j L4
L3:
  addi $sp, $sp, -16
  sw $4, 12($sp)
  sw $5, 8($sp)
  sw $6, 4($sp)
  sw $7, 0($sp)
  move $a0, $9
  addi $sp, $sp, -72
  sw $8, 68($sp)
  sw $9, 64($sp)
  sw $10, 60($sp)
  sw $11, 56($sp)
  sw $12, 52($sp)
  sw $13, 48($sp)
  sw $14, 44($sp)
  sw $15, 40($sp)
  sw $16, 36($sp)
  sw $17, 32($sp)
  sw $18, 28($sp)
  sw $19, 24($sp)
  sw $20, 20($sp)
  sw $21, 16($sp)
  sw $22, 12($sp)
  sw $23, 8($sp)
  sw $24, 4($sp)
  sw $25, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  jal fact
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $25, 0($sp)
  lw $24, 4($sp)
  lw $23, 8($sp)
  lw $22, 12($sp)
  lw $21, 16($sp)
  lw $20, 20($sp)
  lw $19, 24($sp)
  lw $18, 28($sp)
  lw $17, 32($sp)
  lw $16, 36($sp)
  lw $15, 40($sp)
  lw $14, 44($sp)
  lw $13, 48($sp)
  lw $12, 52($sp)
  lw $11, 56($sp)
  lw $10, 60($sp)
  lw $9, 64($sp)
  lw $8, 68($sp)
  addi $sp, $sp, 72
  addi $sp, $sp, 0
  lw $7, 0($sp)
  lw $6, 4($sp)
  lw $5, 8($sp)
  lw $4, 12($sp)
  addi $sp, $sp, 16
  move $11, $v0
  move $8, $11
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
  li $9, 0
  move $v0, $9
  jr $ra
