// This test verifies that S-mode reading instret traps as illegal
// instruction when mcounteren.IR=0 (access not granted)
//
#include <stdint.h>
#include "bp_utils.h"

void trap_handler() {
  uint64_t scause;
  __asm__ __volatile__ ("csrr %0, scause" : "=r" (scause));
  if (scause != 2)
    bp_finish(1);
  bp_finish(0);
}

void s_mode_main() {
  uint64_t val;
  __asm__ __volatile__ ("csrr %0, instret" : "=r" (val));
  bp_finish(1);
}

int main(uint64_t argc, char * argv[]) {
  __asm__ __volatile__ ("csrw mcounteren, %0" : : "r" (0));
  __asm__ __volatile__ ("csrw stvec, %0" : : "r" (&trap_handler));

  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus = (mstatus & ~(3ULL << 11)) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  __asm__ __volatile__ ("csrw satp, %0" : : "r" (0));
  __asm__ __volatile__ ("csrw mepc, %0" : : "r" (&s_mode_main));
  __asm__ __volatile__ ("mret");

  bp_finish(1);
}
