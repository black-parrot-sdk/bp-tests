# Regression test for black-parrot/black-parrot#1360.
# Under Hit-Under-Miss (e_cfg_hit_under_miss = 1), a younger SC must not retire
# ahead of an older in-flight cache-missing LR in the MSHR, or else the late LR
# fill replay sets load_reserved_v_r = 1 after SC #1 has already retired and
# allows a second SC (#2, with no intervening LR) to illegally succeed (rd = 0).

.section .text
.globl main
.option norvc

main:
    la t0, fail
    csrw mtvec, t0

    la t5, cold_target_line
    li t4, 0x1122334455667788
    li t6, 0xaabbccddeeff0011
    li s2, 0xff
    li s3, 0xff

    # 1. LR.D on a cold cache line misses in L1 D-Cache (MSHR latches lr_op = 1).
    lr.d t2, (t5)

    # 2. Back-to-back SC.D #1 (no register dependency on t2).
    sc.d s2, t4, (t5)

    # 3. Wait for the older LR.D fill to complete in L1 D-Cache.
    li a0, 8
1:
    addi a0, a0, -1
    bnez a0, 1b

    # 4. Second SC.D #2 with no intervening LR.D since SC.D #1.
    #    Per RISC-V Zalrsc (Section 8.2), an SC can never succeed without a
    #    prior LR since the most recent SC (s3 must be non-zero).
    sc.d s3, t6, (t5)
    beqz s3, fail

pass:
    li t0, 0x00102000
    sw zero, 0(t0)
2:
    wfi
    j 2b

.align 2
fail:
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
3:
    wfi
    j 3b

.section .data
.balign 4096
cold_target_line:
    .dword 0
    .space 4088
