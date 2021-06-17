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
  addi $sp, $sp, -4
  sw $4, 0($sp)
  j L1
L0:
  lw $8, 0($sp)
  move $v0, $8
  jr $ra
  j L2
L1:
  addi $8, $8, -1
  sw $8, 0($sp)
  addi $sp, $sp, -4
  sw $8, 0($sp)
  lw $8, 0($sp)
  move $a0, $8
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $8, $v0
  addi $sp, $sp, -4
  sw $8, 0($sp)
  lw $8, 8($sp)
  lw $9, 0($sp)
  mul $10, $8, $9
  sw $8, 8($sp)
  sw $9, 0($sp)
  addi $sp, $sp, -4
  sw $10, 0($sp)
  lw $8, 0($sp)
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
  addi $sp, $sp, -4
  sw $8, 0($sp)
  lw $9, 0($sp)
  move $8, $9
  sw $9, 0($sp)
  addi $sp, $sp, -4
  sw $8, 0($sp)
  lw $8, 0($sp)
  li $9, 1
  bgt $8, $9, L3
  sw $8, 0($sp)
  j L4
L3:
  lw $8, 0($sp)
  move $a0, $8
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $8, $v0
  addi $sp, $sp, -4
  sw $8, 0($sp)
  lw $9, 0($sp)
  move $8, $9
  sw $9, 0($sp)
  addi $sp, $sp, -4
  sw $8, 0($sp)
  j L5
L4:
  lw $8, 0($sp)
  li $8, 1
  sw $8, 0($sp)
L5:
  lw $8, 0($sp)
  move $a0, $8
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  sw $8, 0($sp)
  li $8, 0
  move $v0, $8
  jr $ra
