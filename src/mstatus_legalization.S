.section .text
.globl main

main:
    # Load the mask for mstatus.MPP (bits 11 and 12)
    # 0x1800 = 0b0001 1000 0000 0000
    li t0, 0x1800
    
    # Clear the current MPP field
    csrr t1, mstatus
    not t2, t0
    and t1, t1, t2
    
    # Set the MPP field to 2 
    # (2 << 11 = 0x1000 = 0b0001 0000 0000 0000)
    li t3, 0x1000
    or t1, t1, t3
    
    # Write the modified value (with MPP=2) to mstatus
    csrw mstatus, t1
    
    # Read mstatus back to see what the hardware legalized it to
    # anything but hypervisor is fine
    csrr t4, mstatus
    and t4, t4, t0
    li t5, 2
    bne t4, t5, test_pass

test_fail:
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
2:
    wfi
    j 2b

test_pass:
    li t0, 0x00102000
    li t1, 0
    sw t1, 0(t0)
2:
    wfi
    j 2b

