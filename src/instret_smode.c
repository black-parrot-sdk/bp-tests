#include <stdint.h>
#include "bp_utils.h"

void m_trap_handler() {
  uint64_t mcause;
  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));

  // Expected path: S-mode ecall after successful instret read.
  if (mcause == 9)
    bp_finish(0);

  bp_finish(1);
}

void s_mode_main() {
  uint64_t val;
  __asm__ __volatile__ ("csrr %0, instret" : "=r" (val));
  __asm__ __volatile__ ("ecall");
  bp_finish(1);
}

int main(uint64_t argc, char * argv[]) {
  // grant S-mode instret access via mcounteren.IR (bit 2)
  __asm__ __volatile__ ("csrw mcounteren, %0" : : "r" (1ULL << 2));

  // set mtvec to M-mode trap handler
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&m_trap_handler));

  // mstatus.mpp = 01 (S-mode)
  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus = (mstatus & ~(3ULL << 11)) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  // disable VM
  __asm__ __volatile__ ("csrw satp, %0" : : "r" (0));

  // mret into S-mode at s_mode_main
  __asm__ __volatile__ ("csrw mepc, %0" : : "r" (&s_mode_main));
  __asm__ __volatile__ ("mret");

  bp_finish(1);
}
