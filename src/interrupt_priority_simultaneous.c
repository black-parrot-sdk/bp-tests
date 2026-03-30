// This test checks M-mode interrupt priority when all sources are pending together.
#include <stdint.h>
#include "bp_utils.h"

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
    clint_set_mtimecmp(~0ull);
    bp_finish(0);
  }

  bp_finish(1);
}

int main(uint64_t argc, char * argv[]) {
  __asm__ __volatile__ ("csrw mtvec, %0" : : "r" (&trap_handler));

  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus &= ~(1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  // Enable all machine interrupt classes up front.
  uint64_t mie = (1ULL << 3) | (1ULL << 7) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mie, %0" : : "r" (mie));

  // Clear stale state.
  clint_set_msip(0);
  clint_set_meip(0);
  clint_set_mtimecmp(~0ull);

  // Arm all pending sources before turning on MIE.
  clint_set_msip(1);
  clint_set_meip(1);
  clint_set_mtimecmp(clint_read_mtime());

  __asm__ __volatile__ ("fence iorw, iorw" : : : "memory");

  mstatus |= (1ULL << 3);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  for (uint64_t i = 0; i < 1000000; i++) { }

  bp_finish(1);
}