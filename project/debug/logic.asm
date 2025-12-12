  .text
  .globl main
main:
  addi sp, sp, -16
.entry:
  li t0, 1
  li t1, 2
  sgt t0, t0, t1
  seqz t0, t0
  sw t0, 0(sp)
  lw a0, 0(sp)
  addi sp, sp, 16
  ret
