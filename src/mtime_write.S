# Regression for black-parrot: a write to the memory-mapped mtime register
# (CLINT 0x30bff8) must take effect.  The privileged spec describes mtime as a
# machine-mode read-write register.  BlackParrot used to drop the write and let
# the counter free run.
#
# Cases: (1) mtime reads back at or above the written value and below it plus
# a bound, (2) it keeps counting, (3) MTIP is clear while mtime is below
# mtimecmp, (4) a write above mtimecmp sets mip.MTIP, (5) a write back below
# clears it, (6) a write above mtimecmp takes the timer interrupt.
#
# The program terminates through BlackParrot's host "finish" MMIO register:
# store 0 -> PASS, store the number of the failing case -> FAIL.

    .equ MTIME,    0x30bff8
    .equ MTIMECMP, 0x304000
    .equ BIG,      0x100000000000
    .equ BOUND,    0x10000

    .macro SETTLE
    .rept 64
    nop
    .endr
    .endm

    .section .text.init, "ax"
    .globl _start
_start:
    la      t0, trap_handler
    csrw    mtvec, t0
    li      s0, MTIME
    li      s1, MTIMECMP

    # 1: write a large value, read back within [BIG, BIG + BOUND)
    li      s7, 1
    li      t0, BIG
    sd      t0, 0(s0)
    ld      a0, 0(s0)
    bltu    a0, t0, fail
    li      t1, BOUND
    add     t1, t1, t0
    bgeu    a0, t1, fail

    # 2: the counter keeps running after the write
    li      s7, 2
    li      t2, 0x100000            # generous bound, rt_clk may be slow
3:
    ld      a1, 0(s0)
    bgtu    a1, a0, 4f
    addi    t2, t2, -1
    bnez    t2, 3b
    j       fail
4:

    # 3: MTIP follows mtime >= mtimecmp
    li      s7, 3
    li      t0, 0x2000000000000000
    sd      t0, 0(s1)               # mtimecmp
    SETTLE
    csrr    t1, mip
    andi    t1, t1, 0x80
    bnez    t1, fail                # BIG < mtimecmp: MTIP clear
    li      s7, 4
    li      t0, 0x3000000000000000
    sd      t0, 0(s0)               # mtime above mtimecmp
    SETTLE
    csrr    t1, mip
    andi    t1, t1, 0x80
    beqz    t1, fail                # MTIP set
    li      s7, 5
    li      t0, BIG
    sd      t0, 0(s0)               # back below mtimecmp
    SETTLE
    csrr    t1, mip
    andi    t1, t1, 0x80
    bnez    t1, fail                # MTIP clear again

    # 6: a write above mtimecmp takes the timer interrupt
    li      s7, 6
    li      s8, 0
    li      t0, 0x80
    csrw    mie, t0                 # MTIE
    csrsi   mstatus, 0x8            # MIE
    li      t0, 0x3000000000000000
    sd      t0, 0(s0)
    SETTLE
    SETTLE
    j       fail                    # the interrupt should have fired
1:
    li      s7, 0                   # PASS
fail:
    li      t0, 0x00102000          # BlackParrot host finish register
    sd      s7, 0(t0)
2:  j       2b

    .align 2
trap_handler:
    csrr    t0, mcause
    li      t1, 0x8000000000000007  # machine timer interrupt
    bne     t0, t1, fail
    csrw    mie, zero               # MTIP stays pending
    la      s11, 1b
    csrw    mepc, s11
    li      t0, 0x1800              # MPP = M
    csrs    mstatus, t0
    mret
