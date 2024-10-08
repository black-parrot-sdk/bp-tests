# define STORE    sd
# define LOAD     ld
# define REGBYTES 8

#define STACK_TOP 0x800F0000
#define SIZEOF_TRAPFRAME_T 35*REGBYTES

  .section ".text.init","ax",@progbits
  .globl _start
  .align 2
_start:
  /* Setup trap handler */
  la t0, m_trap_vector
  csrw mtvec, t0
  /* Setup stack pointer */
  li sp, STACK_TOP - SIZEOF_TRAPFRAME_T
  csrw mscratch, sp

  /* 0 all registers */
  addi x1,  x0, 0
  addi x3,  x0, 0
  addi x4,  x0, 0
  addi x5,  x0, 0
  addi x6,  x0, 0
  addi x7,  x0, 0
  addi x8,  x0, 0
  addi x9,  x0, 0
  addi x10, x0, 0
  addi x11, x0, 0
  addi x12, x0, 0
  addi x13, x0, 0
  addi x14, x0, 0
  addi x15, x0, 0
  addi x16, x0, 0
  addi x17, x0, 0
  addi x18, x0, 0
  addi x19, x0, 0
  addi x20, x0, 0
  addi x21, x0, 0
  addi x22, x0, 0
  addi x23, x0, 0
  addi x24, x0, 0
  addi x25, x0, 0
  addi x26, x0, 0
  addi x27, x0, 0
  addi x28, x0, 0
  addi x29, x0, 0
  addi x30, x0, 0
  addi x31, x0, 0
  j main

// Test programs can override m_handle_u_ecall to implement custom M-mode syscalls.
// a1 is used as syscall number: 0 is bp_finish, others defer to m_handle_u_ecall.
// m_handle_u_ecall must correctly save and restore callee-saved regs.
  .align 2
  .weak m_handle_u_ecall
m_handle_u_ecall:
  // default ecall handler: bp_finish
  j bp_finish

// Machine-mode trap handler
  .align 2
m_trap_vector:
  csrrw sp, mscratch, sp
  STORE t0,5*REGBYTES(sp)
  STORE t1,6*REGBYTES(sp)

  // Defer to perch's handler if not an ECALL
  li t0, 8
  csrr t1, mcause
  bne t0, t1, 1f

  // use a1 for syscall number; 0 is finish, anything else defers to m_handle_u_ecall weak symbol
  beqz a1, bp_finish

  call m_handle_u_ecall

  // return to ECALL site after handling
  csrr t0, mepc
  addi t0, t0, 4
  csrw mepc, t0

  LOAD t0,5*REGBYTES(sp)
  LOAD t1,6*REGBYTES(sp)
  csrrw sp, mscratch, sp
  mret

1:
  LOAD t0,5*REGBYTES(sp)
  LOAD t1,6*REGBYTES(sp)
  csrrw sp, mscratch, sp
  j __mtvec_handler

  .globl  pop_tf
pop_tf:
  LOAD  t0,33*REGBYTES(a0)
  csrw  sepc,t0
  LOAD  x1,1*REGBYTES(a0)
  LOAD  x2,2*REGBYTES(a0)
  LOAD  x3,3*REGBYTES(a0)
  LOAD  x4,4*REGBYTES(a0)
  LOAD  x5,5*REGBYTES(a0)
  LOAD  x6,6*REGBYTES(a0)
  LOAD  x7,7*REGBYTES(a0)
  LOAD  x8,8*REGBYTES(a0)
  LOAD  x9,9*REGBYTES(a0)
  LOAD  x11,11*REGBYTES(a0)
  LOAD  x12,12*REGBYTES(a0)
  LOAD  x13,13*REGBYTES(a0)
  LOAD  x14,14*REGBYTES(a0)
  LOAD  x15,15*REGBYTES(a0)
  LOAD  x16,16*REGBYTES(a0)
  LOAD  x17,17*REGBYTES(a0)
  LOAD  x18,18*REGBYTES(a0)
  LOAD  x19,19*REGBYTES(a0)
  LOAD  x20,20*REGBYTES(a0)
  LOAD  x21,21*REGBYTES(a0)
  LOAD  x22,22*REGBYTES(a0)
  LOAD  x23,23*REGBYTES(a0)
  LOAD  x24,24*REGBYTES(a0)
  LOAD  x25,25*REGBYTES(a0)
  LOAD  x26,26*REGBYTES(a0)
  LOAD  x27,27*REGBYTES(a0)
  LOAD  x28,28*REGBYTES(a0)
  LOAD  x29,29*REGBYTES(a0)
  LOAD  x30,30*REGBYTES(a0)
  LOAD  x31,31*REGBYTES(a0)
  LOAD  a0,10*REGBYTES(a0)
  sret

  .global  trap_entry
  .align 2
trap_entry:
  csrrw sp, sscratch, sp

  # save gprs
  STORE  x1,1*REGBYTES(sp)
  STORE  x3,3*REGBYTES(sp)
  STORE  x4,4*REGBYTES(sp)
  STORE  x5,5*REGBYTES(sp)
  STORE  x6,6*REGBYTES(sp)
  STORE  x7,7*REGBYTES(sp)
  STORE  x8,8*REGBYTES(sp)
  STORE  x9,9*REGBYTES(sp)
  STORE  x10,10*REGBYTES(sp)
  STORE  x11,11*REGBYTES(sp)
  STORE  x12,12*REGBYTES(sp)
  STORE  x13,13*REGBYTES(sp)
  STORE  x14,14*REGBYTES(sp)
  STORE  x15,15*REGBYTES(sp)
  STORE  x16,16*REGBYTES(sp)
  STORE  x17,17*REGBYTES(sp)
  STORE  x18,18*REGBYTES(sp)
  STORE  x19,19*REGBYTES(sp)
  STORE  x20,20*REGBYTES(sp)
  STORE  x21,21*REGBYTES(sp)
  STORE  x22,22*REGBYTES(sp)
  STORE  x23,23*REGBYTES(sp)
  STORE  x24,24*REGBYTES(sp)
  STORE  x25,25*REGBYTES(sp)
  STORE  x26,26*REGBYTES(sp)
  STORE  x27,27*REGBYTES(sp)
  STORE  x28,28*REGBYTES(sp)
  STORE  x29,29*REGBYTES(sp)
  STORE  x30,30*REGBYTES(sp)
  STORE  x31,31*REGBYTES(sp)

  csrrw  t0,sscratch,sp
  STORE  t0,2*REGBYTES(sp)

  # get sr, epc, badvaddr, cause
  csrr   t0,sstatus
  STORE  t0,32*REGBYTES(sp)
  csrr   t0,sepc
  STORE  t0,33*REGBYTES(sp)
  csrr   t0,stval
  STORE  t0,34*REGBYTES(sp)
  csrr   t0,scause
  STORE  t0,35*REGBYTES(sp)

  move  a0, sp
  j handle_trap
