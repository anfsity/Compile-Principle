  .text
  .globl main
main:
  addi sp, sp, -112
entry:
  li t0, 10
  sw t0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw t0, 4(sp)
  li t1, 1
  sgt t0, t0, t1
  sw t0, 8(sp)
  lw t0, 8(sp)
  bnez t0, then_0
  j end_0
then_0:
  lw t0, 0(sp)
  sw t0, 12(sp)
  lw t0, 12(sp)
  li t1, 1
  sub t0, t0, t1
  sw t0, 16(sp)
  lw t0, 16(sp)
  sw t0, 0(sp)
  li t0, 5
  sw t0, 20(sp)
  lw t0, 20(sp)
  sw t0, 24(sp)
  li t0, 0
  li t1, 1
  sub t0, t0, t1
  sw t0, 28(sp)
  lw t0, 24(sp)
  lw t1, 28(sp)
  slt t0, t0, t1
  sw t0, 32(sp)
  lw t0, 32(sp)
  bnez t0, then_1
  j else_1
end_0:
  lw t0, 0(sp)
  sw t0, 36(sp)
  lw t0, 36(sp)
  li t1, 9
  xor t0, t0, t1
  seqz t0, t0
  sw t0, 40(sp)
  lw t0, 40(sp)
  bnez t0, then_2
  j end_2
then_1:
  li a0, 10
  addi sp, sp, 112
  ret
else_1:
  li t0, 98
  sw t0, 44(sp)
  j end_1
then_2:
  lw t0, 0(sp)
  sw t0, 52(sp)
  lw t0, 52(sp)
  li t1, 1
  sub t0, t0, t1
  sw t0, 56(sp)
  lw t0, 56(sp)
  sw t0, 48(sp)
  lw t0, 48(sp)
  sw t0, 64(sp)
  lw t0, 64(sp)
  li t1, 1
  sub t0, t0, t1
  sw t0, 68(sp)
  lw t0, 68(sp)
  sw t0, 60(sp)
  lw t0, 60(sp)
  sw t0, 72(sp)
  lw t0, 48(sp)
  sw t0, 76(sp)
  lw t0, 72(sp)
  lw t1, 76(sp)
  xor t0, t0, t1
  snez t0, t0
  sw t0, 80(sp)
  lw t0, 80(sp)
  bnez t0, then_3
  j else_3
end_2:
  li t0, 0
  li t1, 1
  sub t0, t0, t1
  sw t0, 84(sp)
  lw a0, 84(sp)
  addi sp, sp, 112
  ret
end_1:
  j end_0
then_3:
  lw t0, 60(sp)
  sw t0, 88(sp)
  li t0, 0
  lw t1, 88(sp)
  xor t0, t0, t1
  seqz t0, t0
  sw t0, 92(sp)
  lw t0, 92(sp)
  bnez t0, then_4
  j end_4
else_3:
  lw t0, 48(sp)
  sw t0, 96(sp)
  lw a0, 96(sp)
  addi sp, sp, 112
  ret
then_4:
  li a0, 0
  addi sp, sp, 112
  ret
end_4:
  lw t0, 60(sp)
  sw t0, 100(sp)
  lw a0, 100(sp)
  addi sp, sp, 112
  ret
