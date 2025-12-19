  .text
  .globl main
main:
  addi sp, sp, -16
.entry:
  li t0, 2
  sw t0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw a0, 4(sp)
  addi sp, sp, 16
  ret
