.section .text
    .global main

main:
    # ------------------------------------------------------------------
    # SETUP
    # ------------------------------------------------------------------
    la t0, m_trap_vector
    csrw mtvec, t0

    # Set mstatus.MPP to S-mode (0x0800)
    csrr t2, mstatus
    li t0, ~0x1800
    and t2, t2, t0
    li t0, 0x0800
    or t2, t2, t0
    csrw mstatus, t2

    # Jump to the first M-mode phase
    j m_phase1

    # ------------------------------------------------------------------
    # PHASE 1: Software Injected, Un-delegated
    # ------------------------------------------------------------------
m_phase1:
    li t1, 0x200            # Bit 9 mask (SEI)
    csrc mideleg, t1        # Un-delegate
    csrs mip, t1            # Inject SW interrupt

    la t0, s_phase1
    csrw mepc, t0
    mret

s_phase1:
    csrr t0, sip
    andi t0, t0, 0x200
    bnez t0, test_fail      # FAIL: Un-delegated SW interrupt leaked!
    
    la a0, m_phase2         # Tell M-mode trap handler where to go next
    ecall

    # ------------------------------------------------------------------
    # PHASE 2: Software Injected, Delegated
    # ------------------------------------------------------------------
m_phase2:
    li t1, 0x200
    csrs mideleg, t1        # Delegate it

    la t0, s_phase2
    csrw mepc, t0
    mret

s_phase2:
    csrr t0, sip
    andi t0, t0, 0x200
    beqz t0, test_fail      # FAIL: Delegated SW interrupt hidden!
    
    la a0, m_phase3
    ecall

    # ------------------------------------------------------------------
    # PHASE 3: Hardware Line, Un-delegated
    # ------------------------------------------------------------------
m_phase3:
    li t1, 0x200
    csrc mideleg, t1        # Un-delegate
    csrc mip, t1            # Clear the SW interrupt
    
    # Assert the hardware interrupt line via memory map
    li t2, 0x30c008
    li t3, 1
    sw t3, 0(t2)

    la t0, s_phase3
    csrw mepc, t0
    mret

s_phase3:
    csrr t0, sip
    andi t0, t0, 0x200
    bnez t0, test_fail      # FAIL: Un-delegated HW interrupt leaked!
    
    la a0, m_phase4
    ecall

    # ------------------------------------------------------------------
    # PHASE 4: Hardware Line, Delegated
    # ------------------------------------------------------------------
m_phase4:
    li t1, 0x200
    csrs mideleg, t1        # Delegate it

    la t0, s_phase4
    csrw mepc, t0
    mret

s_phase4:
    csrr t0, sip
    andi t0, t0, 0x200
    beqz t0, test_fail      # FAIL: Delegated HW interrupt hidden!
    
    # Clean up: De-assert the hardware interrupt
    li t2, 0x30c004
    sw zero, 0(t2)

    j test_pass

    # ------------------------------------------------------------------
    # M-MODE TRAP HANDLER
    # ------------------------------------------------------------------
    .balign 4
m_trap_vector:
    # Verify trap was an ecall from S-mode (mcause == 9)
    csrr t0, mcause
    li t1, 9
    bne t0, t1, test_fail   # FAIL: Unexpected trap cause
    
    # Advance the state machine by jumping to the address in a0
    jr a0

    # ------------------------------------------------------------------
    # END STATES
    # ------------------------------------------------------------------
test_fail:
    li t0, 0x00102000
	li t1, 1
    sw t1, 0(t0)
1:
    wfi
    j 1b

test_pass:
    li t0, 0x00102000
    sw zero, 0(t0)
1:
    wfi
    j 1b

