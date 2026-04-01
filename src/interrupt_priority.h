
#ifndef INTERRUPT_PRIORITY_H
#define INTERRUPT_PRIORITY_H

// CLINT/PLIC base and offsets (see bp_common_clint_pkgdef.svh)
#define CLINT_BASE_ADDR        0x00300000ull
#define CLINT_MIPI_OFFSET      0x0000ull
#define CLINT_MTIMECMP_OFFSET  0x4000ull
#define CLINT_MTIME_OFFSET     0xBFF8ull
#define CLINT_PLIC_OFFSET      0xC000ull
#define CLINT_PLIC_STRIDE      0x0008ull

__attribute__((always_inline)) static inline void mmio_write64(uint64_t addr, uint64_t val) {
  *(volatile uint64_t *)addr = val;
}

__attribute__((always_inline)) static inline void mmio_write32(uint64_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

__attribute__((always_inline)) static inline uint64_t mmio_read64(uint64_t addr) {
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

#endif
