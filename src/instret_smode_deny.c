// This test verifies that S-mode reading instret traps as illegal
// instruction when mcounteren.IR=0 (access not granted)
//
#include <stdint.h>
#include "bp_utils.h"

void m_trap_handler() {
  uint64_t mcause;
  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));

  // Expected path: S-mode illegal instruction on instret read.
  if (mcause != 2)
    bp_finish(1);
  bp_finish(0);
}

void s_mode_main() {
  uint64_t val;
  __asm__ __volatile__ ("csrr %0, instret" : "=r" (val));
  // If we get here, the instret read did not trap as expected.
  __asm__ __volatile__ ("ecall");
  bp_finish(1);
}

int main(uint64_t argc, char * argv[]) {
  __asm__ __volatile__ ("csrw mcounteren, %0" : : "r" (0));
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&m_trap_handler));

  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus = (mstatus & ~(3ULL << 11)) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  __asm__ __volatile__ ("csrw satp, %0" : : "r" (0));
  __asm__ __volatile__ ("csrw mepc, %0" : : "r" (&s_mode_main));
  __asm__ __volatile__ ("mret");

  bp_finish(1);
}
