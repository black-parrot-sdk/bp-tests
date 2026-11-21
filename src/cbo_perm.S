# Regression for black-parrot#1366: a cache-block management instruction
# (cbo.clean, cbo.flush, cbo.inval) is permitted whenever a load or a store
# would be, and does not check or set the dirty bit (Zicbom, riscv-isa-manual
# cmo.adoc: "A cache-block management instruction is permitted to access the
# specified cache block whenever a load instruction or store instruction is
# permitted" and "a cache-block management instruction does not check the
# dirty bit").  BlackParrot used to treat them as plain stores and took a store
# page fault on a read-only leaf and on a writable leaf with D=0.
#
# When access is not permitted they raise a store/AMO page fault (cause 15)
# with the virtual address in stval/mtval.  cbo.zero is a store and keeps the
# store rules.  Every case runs once through the page walk (after sfence.vma)
# and once through a TLB hit.  The test runs in S-mode under Sv39.
#
# The program terminates through BlackParrot's host "finish" MMIO register:
# store 0 -> PASS, store the number of the failing case -> FAIL.

    .equ LEAF_R,    0x43            # V|R|A           read-only
    .equ LEAF_RW,   0x47            # V|R|W|A         writable, D=0
    .equ LEAF_RWD,  0xC7            # V|R|W|A|D
    .equ LEAF_U,    0xD7            # V|R|W|U|A|D     user page, SUM=0

    # expect no trap
    .macro NOTRAP num
    li      s7, \num
    li      s9, -1
    .endm

    # expect a trap with the given cause and tval, then resume at 1f
    .macro EXPECT num, cause, tval
    li      s7, \num
    li      s9, \cause
    li      s10, \tval
    la      s11, 1f
    .endm

    # clean, flush, inval on one page; case numbers num..num+2, none may trap
    .macro CBO_OK num, va
    li      a1, \va
    NOTRAP  \num
    cbo.clean (a1)
    NOTRAP  \num+1
    cbo.flush (a1)
    NOTRAP  \num+2
    cbo.inval (a1)
    .endm

    # same, each one must take a store page fault
    .macro CBO_FAULT num, va
    li      a1, \va
    EXPECT  \num, 15, \va
    cbo.clean (a1)
    j       fail
1:
    EXPECT  \num+1, 15, \va
    cbo.flush (a1)
    j       fail
1:
    EXPECT  \num+2, 15, \va
    cbo.inval (a1)
    j       fail
1:
    .endm

    .section .text.init, "ax"
    .globl _start
_start:
    li      s7, 1                   # setup failed
    li      s9, -1
    la      t0, trap_handler
    csrw    mtvec, t0
    csrw    medeleg, zero
    csrw    mideleg, zero

    # Sv39: root[0] -> L1 -> L0, root[2] = gigapage identity map of DRAM
    la      t1, pt_root
    la      t0, pt_l1
    srli    t0, t0, 12
    slli    t0, t0, 10
    ori     t0, t0, 1
    sd      t0, 0(t1)
    li      t0, (0x80000 << 10) | 0xCF  # V|R|W|X|A|D
    sd      t0, 16(t1)

    la      t1, pt_l1
    la      t0, pt_l0
    srli    t0, t0, 12
    slli    t0, t0, 10
    ori     t0, t0, 1
    sd      t0, 0(t1)

    # L0: VA 0x0000 R, 0x1000 RW D=0, 0x2000 RWD, 0x3000 invalid, 0x4000 U
    la      t1, pt_l0
    la      t2, data_page
    srli    t2, t2, 12
    slli    t2, t2, 10
    ori     t0, t2, LEAF_R
    sd      t0, 0(t1)
    ori     t0, t2, LEAF_RW
    sd      t0, 8(t1)
    ori     t0, t2, LEAF_RWD
    sd      t0, 16(t1)
    sd      zero, 24(t1)
    ori     t0, t2, LEAF_U
    sd      t0, 32(t1)

    la      t0, pt_root
    srli    t0, t0, 12
    li      t1, 8 << 60
    or      t0, t0, t1
    csrw    satp, t0
    sfence.vma

    # enter S-mode at s_main
    li      t0, 0x1800
    csrc    mstatus, t0
    li      t0, 0x800               # MPP = S
    csrs    mstatus, t0
    la      t0, s_main
    csrw    mepc, t0
    mret

s_main:
    # 10..: read-only leaf, walk then TLB hit
    sfence.vma
    CBO_OK  10, 0x0000
    CBO_OK  13, 0x0000
    # 20..: writable leaf with D=0
    sfence.vma
    CBO_OK  20, 0x1000
    CBO_OK  23, 0x1000
    # 30..: writable leaf with D=1
    sfence.vma
    CBO_OK  30, 0x2000
    CBO_OK  33, 0x2000

    # 40..: TLB entry installed by a load (no D, no W), then the CBOs
    sfence.vma
    NOTRAP  40
    li      a1, 0x1000
    ld      a0, 0(a1)
    CBO_OK  41, 0x1000

    # 50..: invalid PTE and user page without SUM fault like a store
    sfence.vma
    CBO_FAULT 50, 0x3000
    CBO_FAULT 53, 0x4000

    # 60: cbo.zero is a store: it works on a writable D=1 leaf
    sfence.vma
    NOTRAP  60
    li      a1, 0x2000
    cbo.zero (a1)
    # 61: and faults on a read-only leaf, on the walk
    sfence.vma
    li      a1, 0x0000
    EXPECT  61, 15, 0x0000
    cbo.zero (a1)
    j       fail
1:
    # 62: and on a TLB hit (installed by a load)
    sfence.vma
    NOTRAP  62
    ld      a0, 0(a1)
    EXPECT  62, 15, 0x0000
    cbo.zero (a1)
    j       fail
1:
    li      s7, 0                   # PASS
fail:
    ecall                           # S-mode ecall reports s7 to the handler

    .align 2
trap_handler:
    csrr    t0, mcause
    li      t1, 9                   # ecall from S-mode: finish
    beq     t0, t1, finish
    bne     t0, s9, bad
    csrr    t0, mtval
    bne     t0, s10, bad
    csrw    mepc, s11
    li      t0, 0x1800
    csrc    mstatus, t0
    li      t0, 0x800               # return to S-mode
    csrs    mstatus, t0
    mret
bad:
finish:
    li      t0, 0x00102000          # BlackParrot host finish register
    sd      s7, 0(t0)
2:  j       2b

    .section .pt, "aw", @progbits
    .align 12
pt_root:    .zero 0x1000
pt_l1:      .zero 0x1000
pt_l0:      .zero 0x1000
data_page:  .zero 0x1000
