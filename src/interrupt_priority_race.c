// This test checks M-mode interrupt priority across pending/enable race orderings.
#include <stdint.h>
#include "bp_utils.h"
#include "interrupt_priority.h"

static const uint64_t expected_ecode[6] = {11, 3, 7, 11, 3, 7};
volatile uint64_t step = 0;

void trap_handler(void) __attribute__((interrupt));
void trap_handler(void) {
  uint64_t mcause;
  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));

  if ((mcause >> 63) == 0)
    bp_finish(1);

  if (step >= 6)
    bp_finish(1);

  uint64_t ecode = mcause & 0xfff;
  if (ecode != expected_ecode[step])
    bp_finish(1);

  if (ecode == 11)
    clint_set_meip(0);
  else if (ecode == 3)
    clint_set_msip(0);
  else if (ecode == 7)
    clint_set_mtimecmp(~0ull);
  else
    bp_finish(1);

  step++;

  // After finishing the pending-before-enable phase, exercise enable-before-pending.
  if (step == 3) {
    clint_set_msip(1);
    clint_set_meip(1);
    clint_set_mtimecmp(clint_read_mtime());
    __asm__ __volatile__ ("fence iorw, iorw" : : : "memory");
    return;
  }

  if (step == 6)
    bp_finish(0);
}

int main(uint64_t argc, char * argv[]) {
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&trap_handler));

  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus &= ~(1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  uint64_t mie = (1ULL << 3) | (1ULL << 7) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mie, %0" : : "r" (mie));

  // Clear stale state first.
  clint_set_msip(0);
  clint_set_meip(0);
  clint_set_mtimecmp(~0ull);

  // Phase A: pending-before-enable.
  clint_set_msip(1);
  clint_set_meip(1);
  clint_set_mtimecmp(clint_read_mtime());

  __asm__ __volatile__ ("fence iorw, iorw" : : : "memory");

  mstatus |= (1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  for (uint64_t i = 0; i < 2000000; i++) { }

  bp_finish(1);
}
