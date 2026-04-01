// This test checks M-mode interrupt priority ordering using CLINT/PLIC MMIO.
#include <stdint.h>
#include "bp_utils.h"
#include "interrupt_priority.h"

volatile uint64_t step = 0;

void trap_handler(void) __attribute__((interrupt));
void trap_handler(void) {
  uint64_t mcause;
  __asm__ __volatile__ ("csrr %0, mcause" : "=r" (mcause));

  if ((mcause >> 63) == 0)
    bp_finish(1);

  uint64_t ecode = mcause & 0xfff;

  if (step == 0) {
    if (ecode != 11)
      bp_finish(1);

    clint_set_meip(0);

    // Allow lower-priority interrupts only after MEIP is serviced.
    uint64_t mie;
    __asm__ __volatile__ ("csrr %0, mie" : "=r" (mie));
    mie |= (1ULL << 3) | (1ULL << 7);
    __asm__ __volatile__ ("csrw mie, %0" : : "r" (mie));

    // Arm software and timer interrupts for the next steps.
    clint_set_msip(1);
    clint_set_mtimecmp(clint_read_mtime());

    step = 1;
    return;
  }

  if (step == 1) {
    if (ecode != 3)
      bp_finish(1);

    clint_set_msip(0);
    step = 2;
    return;
  }

  if (step == 2) {
    if (ecode != 7)
      bp_finish(1);

    // Clear MTIP by moving compare far into the future.
    clint_set_mtimecmp(~0ull);
    bp_finish(0);
  }

  bp_finish(1);
}

int main(uint64_t argc, char * argv[]) {
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&trap_handler));

  // Keep global MIE off while programming pending sources.
  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus &= ~(1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  // Enable only external interrupt first.
  uint64_t mie = (1ULL << 11);
  __asm__ __volatile__ ("csrw mie, %0" : : "r" (mie));

  // Clear stale pending interrupts.
  clint_set_msip(0);
  clint_set_meip(0);
  clint_set_mtimecmp(~0ull);

  // Arm only MEIP before enabling MIE.
  clint_set_meip(1);
  __asm__ __volatile__ ("fence iorw, iorw" : : : "memory");

  //// Enable global machine interrupts.
  mstatus |= (1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  for (uint64_t i = 0; i < 100; i++) { __asm__ __volatile__ ("nop"); }

  bp_finish(1);
}
