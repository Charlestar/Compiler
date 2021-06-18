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
  move $a0, $8
  addi $sp, $sp, -4
  sw $8, 0($sp)
  addi $sp, $sp, -4
  sw $9, 0($sp)
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
  lw $9, 0($sp)
  addi $sp, $sp, 4
  lw $8, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, 0
  move $9, $v0
  mul $10, $4, $9
  move $v0, $10
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
  move $a0, $9
  addi $sp, $sp, -4
  sw $8, 0($sp)
  addi $sp, $sp, -4
  sw $9, 0($sp)
  addi $sp, $sp, -4
  sw $11, 0($sp)
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
  lw $11, 0($sp)
  addi $sp, $sp, 4
  lw $9, 0($sp)
  addi $sp, $sp, 4
  lw $8, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, 0
  move $11, $v0
  move $10, $11
  j L5
L4:
  li $10, 1
L5:
  move $a0, $10
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  li $12, 0
  move $v0, $12
  jr $ra
memset(lru, 0, sizeof(lru));