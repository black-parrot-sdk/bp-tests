// This test verifies that mtval is written with the faulting PC
// on instruction exceptions (mcause = 0 or 1)
//
#include <stdint.h>
#include "bp_utils.h"

void trap_handler() {
  uint64_t mcause, mtval, mepc;
  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));
  __asm__ __volatile__ ("csrr %0, mtval"  : "=r" (mtval));
  __asm__ __volatile__ ("csrr %0, mepc"   : "=r" (mepc));

  // both ecode 0 and ecode 1 should write faulting PC to mtval
  if (mcause == 0 || mcause == 1) {
    if (mtval != mepc)
      bp_finish(1);
  } else {
    // unexpected exception
    bp_finish(1);
  }

  bp_finish(0);
}

int main(uint64_t argc, char * argv[]) {
  uint64_t fault_addr = 0x470000000000ULL;
  uint64_t misaligned_addr = 0x80000003ULL;

  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&trap_handler));

  // Test 1: ecode 1 — instr access fault
  __asm__ __volatile__ ("jr %0" : : "r" (fault_addr));

  // Test 2: ecode 0 — instr misaligned
  __asm__ __volatile__ ("jr %0" : : "r" (misaligned_addr));

  bp_finish(1);
}
