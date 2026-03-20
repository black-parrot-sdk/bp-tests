#include <stdint.h>
#include "bp_utils.h"
// Constants derived from Black Parrot PMA logic configuration
#define DRAM_BASE_ADDR 0x80000000ULL
// The Local / MMIO base address used heavily in Black Parrot
#define CLINT_BASE_ADDR 0x03000000ULL
/*
 * Black Parrot PMA (Physical Memory Attributes) Verification Test
 * 
 * This test runs physically on the RISC-V core and exercises 
 * the hardware PMA module by writing to and reading from:
 * 1. Standard DRAM (Cached, Idempotent)
 * 2. Local space (Uncached, Non-idempotent)
 */
int main(uint64_t argc, char * argv[]) {
    
    // ---------------------------------------------------------
    // TEST 1: Standard DRAM Access
    // PMA should route this as Cached and Idempotent.
    // ---------------------------------------------------------
    volatile uint64_t *dram_ptr = (uint64_t *) (DRAM_BASE_ADDR + 0x1000); // 4KB offset
    
    *dram_ptr = 0xDEADBEEFCAFEBABE;
    
    // Read back to verify
    if (*dram_ptr != 0xDEADBEEFCAFEBABE) {
        bp_finish(1); // FAIL
    }
    // ---------------------------------------------------------
    // TEST 2: Local / Uncached Space Access
    // PMA should route this as Uncached and Non-idempotent.
    // We read from a known local architectural register space (CLINT).
    // ---------------------------------------------------------
    volatile uint64_t *clint_ptr = (uint64_t *) CLINT_BASE_ADDR;
    
    // Perform a load. If PMA logic fails to flag it properly as uncached, 
    // it might try to insert the MMIO space into the L1 cache, causing stalls/faults.
    uint64_t mtimecmp_val = *clint_ptr; 
    
    // Dummy check to prevent compiler optimization
    if (mtimecmp_val == 0xFFFFFFFFFFFFFFFF) {
        // Do nothing, just ensure it executes
    }
    // If we reach this point without taking a memory exception, the PMA logic successfully
    // routed the cached vs uncached requests to the proper memory channels.
    
    // ---------------------------------------------------------
    // PASS
    // ---------------------------------------------------------
    bp_finish(0); // PASS
}
