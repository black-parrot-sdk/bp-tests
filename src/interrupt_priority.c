// This test checks M-mode interrupt priority ordering using CLINT/PLIC MMIO.
#include <stdint.h>
#include "bp_utils.h"

// CLINT/PLIC base and offsets (see bp_common_clint_pkgdef.svh)
#define CLINT_BASE_ADDR        0x00300000ull
#define CLINT_MIPI_OFFSET      0x0000ull
#define CLINT_MTIMECMP_OFFSET  0x4000ull
#define CLINT_MTIME_OFFSET     0xBFF8ull
#define CLINT_PLIC_OFFSET      0xB000ull
#define CLINT_PLIC_STRIDE      0x0008ull

static inline void mmio_write64(uint64_t addr, uint64_t val) {
  *(volatile uint64_t *)addr = val;
}

static inline void mmio_write32(uint64_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

static inline uint64_t mmio_read64(uint64_t addr) {
  return *(volatile uint64_t *)addr;
}

static inline void clint_set_msip(uint32_t val) {
  mmio_write32(CLINT_BASE_ADDR + CLINT_MIPI_OFFSET, val);
}

static inline void clint_set_mtimecmp(uint64_t val) {
  mmio_write64(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET, val);
}

static inline uint64_t clint_read_mtime(void) {
  return mmio_read64(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET);
}

static inline void clint_set_meip(uint64_t val) {
  mmio_write64(CLINT_BASE_ADDR + CLINT_PLIC_OFFSET + (0 * CLINT_PLIC_STRIDE), val);
}

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

  // Enable global machine interrupts.
  mstatus |= (1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  for (uint64_t i = 0; i < 1000000; i++) { }

  bp_finish(1);
}
