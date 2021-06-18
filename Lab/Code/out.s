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
  addi $14, $sp, 16
  sub $15, $14, $13
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
  addi $22, $sp, 16
  sub $23, $22, $21
  addi $24, $10, 1
  li $25, 4
  mul $11, $24, $25
  addi $12, $sp, 16
  sub $14, $12, $11
  lw $23, 0($23)
  lw $14, 0($14)
  bgt $23, $14, L9
  j L10
L9:
  addi $sp, $sp, -4
  sw $13, 0($sp)
  li $13, 4
  addi $sp, $sp, -4
  sw $15, 0($sp)
  mul $15, $10, $13
  addi $sp, $sp, -4
  sw $16, 0($sp)
  addi $16, $sp, 28
  addi $sp, $sp, -4
  sw $9, 0($sp)
  sub $9, $16, $15
  addi $sp, $sp, -4
  sw $17, 0($sp)
  lw $17, 0($9)
  addi $sp, $sp, -4
  sw $8, 0($sp)
  li $8, 4
  mul $18, $10, $8
  addi $sp, $sp, -4
  sw $19, 0($sp)
  addi $19, $sp, 44
  sub $20, $19, $18
  addi $22, $10, 1
  addi $sp, $sp, -4
  sw $21, 0($sp)
  li $21, 4
  addi $sp, $sp, -4
  sw $23, 0($sp)
  mul $23, $22, $21
  addi $sp, $sp, -4
  sw $24, 0($sp)
  addi $24, $sp, 56
  sub $25, $24, $23
  move $20, $25
  addi $12, $10, 1
  addi $sp, $sp, -4
  sw $11, 0($sp)
  li $11, 4
  addi $sp, $sp, -4
  sw $14, 0($sp)
  mul $14, $12, $11
  addi $13, $sp, 64
  sub $16, $13, $14
  sw $17, 0($16)
L10:
  addi $sp, $sp, -4
  sw $9, 0($sp)
  addi $9, $10, 1
  move $10, $9
  j L6
L8:
  li $10, 0
  addi $sp, $sp, -4
  sw $15, 0($sp)
  lw $15, 32($sp)
  addi $8, $15, -1
  move $15, $8
  j L3
L5:
  li $15, 0
L11:
  li $19, 5
  blt $15, $19, L12
  j L13
L12:
  addi $sp, $sp, -4
  sw $18, 0($sp)
  li $18, 4
  addi $sp, $sp, -4
  sw $20, 0($sp)
  mul $20, $15, $18
  addi $sp, $sp, -4
  sw $22, 0($sp)
  addi $22, $sp, 84
  sub $21, $22, $20
  lw $24, 0($21)
  move $a0, $24
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  sw $23, 0($sp)
  addi $23, $15, 1
  move $15, $23
  j L11
L13:
  addi $sp, $sp, -4
  sw $25, 0($sp)
  li $25, 0
  move $v0, $25
  jr $ra
