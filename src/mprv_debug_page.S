.option norvc

.section .text.init
.globl _start
_start:
    j entry
    j fail
    j debug_exception
    nop

entry:
    la t0, debug_entered
    ld t1, 0(t0)
    bnez t1, debug_main

    li t1, 1
    sd t1, 0(t0)
    fence rw, rw

    # Raise the local debug interrupt. Debug entry returns to _start.
    li t0, 0x0030d000
    sd t1, 0(t0)
1:
    j 1b

debug_main:
    # Clear the level-sensitive debug interrupt before running the checks.
    li t0, 0x0030d000
    sd zero, 0(t0)

    la t0, root_page_table
    srli t0, t0, 12
    li t1, 8
    slli t1, t1, 60
    or t0, t0, t1
    csrw satp, t0
    sfence.vma

    # MPRV=1 and MPP=U select U-mode permissions for data accesses.
    csrr t0, mstatus
    li t1, (3 << 11) | (1 << 17)
    not t1, t1
    and t0, t0, t1
    li t1, 1 << 17
    or t0, t0, t1
    csrw mstatus, t0

    # dcsr.mprven=0 must make debug-mode accesses ignore MPRV.
    li t0, 1 << 4
    csrc dcsr, t0
    la t0, test_phase
    li t1, 1
    sd t1, 0(t0)
    li t0, 0x80000000
    ld t1, 0(t0)

    # dcsr.mprven=1 must make the same access use U-mode permissions.
    la t0, test_phase
    li t1, 2
    sd t1, 0(t0)
    li t0, 1 << 4
    csrs dcsr, t0
    li t0, 0x80000000
    ld t1, 0(t0)
    j fail

debug_exception:
    # Debug exceptions do not update mcause. The phase identifies the access.
    li t0, 1 << 17
    csrc mstatus, t0
    la t0, test_phase
    ld t0, 0(t0)
    li t1, 2
    bne t0, t1, fail

pass:
    li t0, 0x00102000
    sw zero, 0(t0)
2:
    wfi
    j 2b

fail:
    li t0, 1 << 17
    csrc mstatus, t0
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
3:
    wfi
    j 3b

.section .data
.balign 8
debug_entered:
    .dword 0
test_phase:
    .dword 0

.balign 4096
root_page_table:
    .zero 16
    .dword 0x20000043
    .zero 4072
