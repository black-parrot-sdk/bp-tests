# Regression for black-parrot#1390: a trap redirect that lands in the same
# icache line as an earlier wrong-path fetch must not hit stale data.  The first
# handler instruction then executes as an illegal instruction (mcause 2)
# instead of the U-mode ecall (mcause 8) being taken.
#
# The trap handler is run once so its 64B line is resident in the icache, then
# a U-mode ecall through an Sv39 2M mapping traps into it.  The layout is fixed
# by the .org directives and the linker script: the `j fail_layout` at
# 0x80000020 is the wrong-path fetch; its target at 0x80004f80 shares a
# 64B icache line with the handler at 0x80004fa0.
#
# The program terminates through BlackParrot's host "finish" MMIO register:
# store 0 -> PASS, store nonzero -> FAIL.

    .section .text.init, "ax"
    .globl _start
_start:
    j       main
tramp:                           # PA 0x80000004
    ecall
main:
    la      t0, pt_root
    li      t1, 0x80010000
    bne     t0, t1, fail_layout
    la      t0, tramp
    li      t1, 0x80000004
    bne     t0, t1, fail_layout
    la      t0, trap_handler
    csrw    mtvec, t0
    csrw    medeleg, zero
    csrw    mideleg, zero
    li      t0, 0x8000000000080010
    csrw    satp, t0
    sfence.vma
    la      t0, trap_handler
    li      s11, 0
    jalr    ra, 0(t0)           # warm-up call: the handler's line is now in the icache
    li      s11, 1
    li      t0, 0xe1800
    csrc    mstatus, t0         # MPP=U, MPRV=0
    li      t0, 0xc0000
    csrs    mstatus, t0         # SUM|MXR
    li      t0, 0x80000004
    csrw    mepc, t0
    mret

    .org 0x4f00
fail:
    li      s7, 1
    li      t0, 0x00102000      # host finish
    sd      s7, 0(t0)
2:  j       2b
    .org 0x4f80
fail_layout:                    # wrong-path jump target
    j       fail
    .org 0x4fa0
trap_handler:
    nop                         # first handler instruction
    bnez    s11, hbody          # s11 = 1 only for the real trap
    ret                         # warm-up call returns here
    .org 0x5100
hbody:
    csrr    s2, mcause
    li      s9, 8
    bne     s2, s9, fail
    li      t0, 0x00102000      # host finish
    sd      zero, 0(t0)         # PASS
2:  j       2b

    .section .pt, "aw", @progbits
    .align 12
pt_root:
    .zero 16
    .quad 0x20004401            # [2] -> pt_l1 at 0x80011000
    .zero 4096-24
pt_l1:
    .quad 0x2000005b            # [0] 2M leaf PA 0x80000000, V R X U A
    .zero 4096-8
