# Regression test for black-parrot/black-parrot#1362.
# Integer-destination FP instructions (fcvt.w/wu/l/lu.s/d, feq/flt/fle.s/d,
# fclass.s/d, fmv.x.w/d) targeting rd = x0 (zero) must still select ird_fflags
# rather than frd_fflags when updating fcsr.fflags.

.section .text
.globl main
.option norvc

main:
    la t0, fail
    csrw mtvec, t0

    # Ensure FP unit is enabled (mstatus.FS = Dirty).
    li t0, (3 << 13)
    csrs mstatus, t0
    csrw fcsr, zero

    # Case A: fcvt.w.s on single-precision +Inf (0x7f800000, NaN-boxed)
    # must set fflags.NV (0x10) even when rd = zero (x0).
    li t0, 0xffffffff7f800000
    fmv.d.x fa0, t0

    csrw fflags, zero
    fcvt.w.s t1, fa0, rne
    csrr s2, fflags
    li t2, 0x10
    bne s2, t2, fail

    csrw fflags, zero
    fcvt.w.s zero, fa0, rne
    csrr s3, fflags
    bne s3, t2, fail

    # Case B: fclass.d on double-precision sNaN (0x7ff0000000000001)
    # must never set any exception flags (fflags = 0x00) even when rd = zero (x0).
    li t0, 0x7ff0000000000001
    fmv.d.x fa1, t0

    csrw fflags, zero
    fclass.d t1, fa1
    csrr s4, fflags
    bnez s4, fail

    csrw fflags, zero
    fclass.d zero, fa1
    csrr s5, fflags
    bnez s5, fail

pass:
    li t0, 0x00102000
    sw zero, 0(t0)
1:
    wfi
    j 1b

.align 2
fail:
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
2:
    wfi
    j 2b
