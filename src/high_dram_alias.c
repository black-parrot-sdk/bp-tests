#include <stdint.h>
#include <bp_utils.h>

static inline void store64(volatile uint64_t *addr, uint64_t val) {
  __asm__ __volatile__("sd %1, 0(%0)" : : "r"(addr), "r"(val) : "memory");
}

static inline uint64_t load64(volatile uint64_t *addr) {
  uint64_t val;
  __asm__ __volatile__("ld %0, 0(%1)" : "=r"(val) : "r"(addr) : "memory");
  return val;
}

int main() {
  volatile uint64_t *cfg_reg_dcache_mode = (volatile uint64_t *)0x200408ULL;
  volatile uint64_t *low_addr  = (volatile uint64_t *)0x083000000ULL;
  volatile uint64_t *high_addr = (volatile uint64_t *)0x183000000ULL;

  const uint64_t pattern_low  = 0x1122334455667788ULL;
  const uint64_t pattern_high = 0xAABBCCDDEEFF0011ULL;

  uint64_t value;

  // Force uncached mode so cache state cannot hide aliasing
  store64(cfg_reg_dcache_mode, 0);

  store64(low_addr, pattern_low);
  value = load64(low_addr);
  bp_print_string("low after low write: ");
  bp_hprint_uint64(value);
  bp_cprint('\n');
  if (value != pattern_low)
    bp_finish(1);

  store64(high_addr, pattern_high);
  value = load64(high_addr);
  bp_print_string("high after high write: ");
  bp_hprint_uint64(value);
  bp_cprint('\n');
  if (value != pattern_high)
    bp_finish(1);

  value = load64(low_addr);
  bp_print_string("low after high write: ");
  bp_hprint_uint64(value);
  bp_cprint('\n');
  if (value != pattern_low)
    bp_finish(1);

  value = load64(high_addr);
  bp_print_string("high final: ");
  bp_hprint_uint64(value);
  bp_cprint('\n');
  if (value != pattern_high)
    bp_finish(1);

  bp_finish(0);
  return 0;
}
