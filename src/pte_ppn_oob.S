# Regression for black-parrot#1386: a PTE whose PPN does not fit in the
# physical address must not be truncated.  An access through such a PTE, leaf
# or pointer, raises an access fault of the access type (load 5, store 7,
# fetch 1) with the virtual address in mtval, like the untranslated access to
# that physical address does.  BlackParrot used to drop the upper PPN bits and
# access the aliased page instead.
#
# Loads and stores run in M-mode with mstatus.MPRV=1, MPP=S.  The fetch runs
# in S-mode.  PPN bit 43 (physical address bit 55) is set in the bad PTEs.
#
# The program terminates through BlackParrot's host "finish" MMIO register:
# store 0 -> PASS, store the number of the failing case -> FAIL.

    .equ PTE_PTR, 0x01              # V
    .equ PTE_RW,  0xC7              # V|R|W|A|D
    .equ PTE_X,   0x49              # V|X|A
    .equ MARKER,  0x5eed

    # reg = PTE pointing to sym, with the PPN bits in hi or'ed in
    .macro PTE reg, sym, hi, flags
    la      \reg, \sym
    srli    \reg, \reg, 12
    li      t2, \hi
    or      \reg, \reg, t2
    slli    \reg, \reg, 10
    ori     \reg, \reg, \flags
    .endm

    # M-mode data accesses translate as S-mode
    .macro MPRV_ON
    li      t0, 0x1800
    csrc    mstatus, t0
    li      t0, 0x20800
    csrs    mstatus, t0
    .endm

    .macro MPRV_OFF
    li      t0, 0x20000
    csrc    mstatus, t0
    .endm

    # expect a trap with the given cause and tval, then resume at 1f
    .macro EXPECT num, cause, tval
    li      s7, \num
    li      s9, \cause
    li      s10, \tval
    la      s11, 1f
    .endm

    .section .text.init, "ax"
    .globl _start
_start:
    j       main
    # fetched through VA 0x2000, only reached if the fetch does not fault
s_target:
    ecall

main:
    la      t0, trap_handler
    csrw    mtvec, t0
    csrw    medeleg, zero
    csrw    mideleg, zero

    # Sv39 tables: root[0] -> L1, L1[0] -> L0, L1[1] -> L0 with a bad PPN
    la      t1, pt_root
    PTE     t0, pt_l1, 0, PTE_PTR
    sd      t0, 0(t1)

    la      t1, pt_l1
    PTE     t0, pt_l0, 0, PTE_PTR
    sd      t0, 0(t1)               # VA 0x000000
    PTE     t0, pt_l0, 1 << 43, PTE_PTR
    sd      t0, 8(t1)               # VA 0x200000

    la      t1, pt_l0
    PTE     t0, data_page, 0, PTE_RW
    sd      t0, 0(t1)               # VA 0x0000 -> data page
    PTE     t0, data_page, 1 << 43, PTE_RW
    sd      t0, 8(t1)               # VA 0x1000 -> data page, bad PPN
    PTE     t0, _start, 1 << 43, PTE_X
    sd      t0, 16(t1)              # VA 0x2000 -> this code, bad PPN

    la      t1, data_page
    li      t0, MARKER
    sd      t0, 0(t1)

    la      t0, pt_root
    srli    t0, t0, 12
    li      t1, 8 << 60
    or      t0, t0, t1
    csrw    satp, t0
    sfence.vma

    # 1: a normal leaf still translates
    li      s7, 1
    li      s9, -1                  # no trap expected
    MPRV_ON
    li      a1, 0x0
    ld      a0, 0(a1)
    MPRV_OFF
    li      t0, MARKER
    bne     a0, t0, fail

    # 2: load through a leaf with a bad PPN
    EXPECT  2, 5, 0x1000
    MPRV_ON
    li      a1, 0x1000
    ld      a0, 0(a1)
    MPRV_OFF
    j       fail
1:
    # 3: store through a leaf with a bad PPN
    EXPECT  3, 7, 0x1008
    MPRV_ON
    li      a1, 0x1008
    sd      zero, 0(a1)
    MPRV_OFF
    j       fail
1:
    # 4: load through a pointer with a bad PPN
    EXPECT  4, 5, 0x200000
    MPRV_ON
    li      a1, 0x200000
    ld      a0, 0(a1)
    MPRV_OFF
    j       fail
1:
    # the store of case 3 must not have reached the aliased page
    li      s7, 5
    la      t1, data_page
    ld      a0, 0(t1)
    li      t0, MARKER
    bne     a0, t0, fail

    # 6: fetch in S-mode through a leaf with a bad PPN
    la      a1, s_target
    li      t0, 0xfff
    and     a1, a1, t0
    li      t0, 0x2000
    or      a1, a1, t0
    li      s7, 6
    li      s9, 1
    mv      s10, a1
    la      s11, 1f
    li      t0, 0x1800
    csrc    mstatus, t0
    li      t0, 0x800               # MPP = S
    csrs    mstatus, t0
    csrw    mepc, a1
    mret
1:
    li      s7, 0                   # PASS
fail:
    li      t0, 0x00102000          # BlackParrot host finish register
    sd      s7, 0(t0)
2:  j       2b

    .align 2
trap_handler:
    MPRV_OFF
    csrr    t0, mcause
    bne     t0, s9, fail
    csrr    t0, mtval
    bne     t0, s10, fail
    li      t0, 0x1800              # MPP = M
    csrs    mstatus, t0
    csrw    mepc, s11
    mret

    .section .pt, "aw", @progbits
    .align 12
pt_root:    .zero 0x1000
pt_l1:      .zero 0x1000
pt_l0:      .zero 0x1000
data_page:  .zero 0x1000
