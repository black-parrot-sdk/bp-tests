// This test verifies that mtval is written with the faulting PC
// on an instruction access fault (mcause = 1)
//
#include <stdint.h>
#include "bp_utils.h"

void trap_handler() {
  uint64_t mcause;
  uint64_t mtval;
  uint64_t mepc;

  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));
  __asm__ __volatile__ ("csrr %0, mtval"  : "=r" (mtval));
  __asm__ __volatile__ ("csrr %0, mepc"   : "=r" (mepc));

  // mcause == 1 is instruction access fault
  if (mcause != 1)
    bp_finish(1);

  // mtval should equal mepc on instruction access fault
  if (mtval != mepc)
    bp_finish(1);

  bp_finish(0);
}

int main(uint64_t argc, char * argv[]) {
  // unmapped address — triggers instruction access fault when jumped to
  uint64_t fault_addr = 0x470000000000ULL;

  // Set trap handler
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&trap_handler));

  // Jump to unmapped address
  __asm__ __volatile__ ("jr %0" : : "r" (fault_addr));

  // Should never reach here
  bp_finish(1);
}
