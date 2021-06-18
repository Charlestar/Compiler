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

main:
  addi $sp, $sp, -20
  li $8, 0
  li $9, 5
  li $10, 0
L0:
  li $11, 5
  blt $8, $11, L1
  j L2
L1:
  li $12, 4
  mul $13, $8, $12
  addi $14, $sp, 20
  addi $15, $14, 0
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $16, $v0
  sw $16, 0($15)
  addi $17, $8, 1
  move $8, $17
  j L0
L2:
  move $8, $9
L3:
  li $18, 0
  bgt $8, $18, L4
  j L5
L4:
L6:
  addi $19, $8, -1
  blt $10, $19, L7
  j L8
L7:
  li $20, 4
  mul $21, $10, $20
  addi $22, $sp, 20
  addi $23, $22, -5
  addi $24, $10, 1
  li $25, 4
  mul $11, $24, $25
  addi $12, $sp, 20
  addi $sp, $sp, -4
  sw $13, 0($sp)
  addi $13, $12, -8
  lw $23, 0($23)
  lw $13, 0($13)
  bgt $23, $13, L9
  j L10
L9:
  li $14, 4
  addi $sp, $sp, -4
  sw $15, 0($sp)
  mul $15, $10, $14
  addi $sp, $sp, -4
  sw $16, 0($sp)
  addi $16, $sp, 32
  addi $sp, $sp, -4
  sw $9, 0($sp)
  addi $9, $16, -10
  addi $sp, $sp, -4
  sw $17, 0($sp)
  lw $17, 0($9)
  addi $sp, $sp, -4
  sw $8, 0($sp)
  li $8, 4
  mul $18, $10, $8
  addi $sp, $sp, -4
  sw $19, 0($sp)
  addi $19, $sp, 48
  addi $20, $19, -12
  addi $sp, $sp, -4
  sw $21, 0($sp)
  addi $21, $10, 1
  li $22, 4
  addi $sp, $sp, -4
  sw $23, 0($sp)
  mul $23, $21, $22
  addi $sp, $sp, -4
  sw $24, 0($sp)
  addi $24, $sp, 60
  addi $25, $24, -15
  move $20, $25
  addi $sp, $sp, -4
  sw $11, 0($sp)
  addi $11, $10, 1
  li $12, 4
  addi $sp, $sp, -4
  sw $13, 0($sp)
  mul $13, $11, $12
  addi $14, $sp, 68
  addi $sp, $sp, -4
  sw $15, 0($sp)
  addi $15, $14, -18
  sw $17, 0($15)
L10:
  addi $16, $10, 1
  move $10, $16
  j L6
L8:
  li $10, 0
  addi $sp, $sp, -4
  sw $9, 0($sp)
  lw $9, 32($sp)
  addi $8, $9, -1
  move $9, $8
  j L3
L5:
  li $9, 0
L11:
  addi $sp, $sp, -4
  sw $18, 0($sp)
  li $18, 5
  blt $9, $18, L12
  j L13
L12:
  li $19, 4
  addi $sp, $sp, -4
  sw $20, 0($sp)
  mul $20, $9, $19
  addi $sp, $sp, -4
  sw $21, 0($sp)
  addi $21, $sp, 88
  addi $22, $21, -22
  addi $sp, $sp, -4
  sw $23, 0($sp)
  lw $23, 0($22)
  move $a0, $23
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $24, $9, 1
  move $9, $24
  j L11
L13:
  addi $sp, $sp, -4
  sw $25, 0($sp)
  li $25, 0
  move $v0, $25
  jr $ra
