#include <stdint.h>
#include <bp_utils.h>

static inline void pass(void) { bp_finish(0); }
static inline void fail(void) { bp_finish(1); }

int main(uint64_t argc, char *argv[])
{
  uint64_t pmpcfg0;
  uint64_t pmpaddr0;

  // Start from a known state for entry 0.
  __asm__ __volatile__("csrw pmpcfg0, x0");
  __asm__ __volatile__("csrw pmpaddr0, x0");

  __asm__ __volatile__("csrr %0, pmpcfg0" : "=r"(pmpcfg0));
  if (pmpcfg0 != 0)
    fail();

  // Reserved encoding R=0,W=1 should be normalized by hardware to W=0.
  pmpcfg0 = 0x2;
  __asm__ __volatile__("csrw pmpcfg0, %0" : : "r"(pmpcfg0));
  __asm__ __volatile__("csrr %0, pmpcfg0" : "=r"(pmpcfg0));
  if (pmpcfg0 & 0x2)
    fail();

  // Ensure address register is writable before lock.
  pmpaddr0 = 0x12345;
  __asm__ __volatile__("csrw pmpaddr0, %0" : : "r"(pmpaddr0));
  __asm__ __volatile__("csrr %0, pmpaddr0" : "=r"(pmpaddr0));
  if (pmpaddr0 != 0x12345)
    fail();

  // Set lock bit for entry 0.
  pmpcfg0 = 0x81;
  __asm__ __volatile__("csrw pmpcfg0, %0" : : "r"(pmpcfg0));
  __asm__ __volatile__("csrr %0, pmpcfg0" : "=r"(pmpcfg0));
  if ((pmpcfg0 & 0x80) == 0)
    fail();

  // Write should be ignored once locked.
  pmpaddr0 = 0x54321;
  __asm__ __volatile__("csrw pmpaddr0, %0" : : "r"(pmpaddr0));
  __asm__ __volatile__("csrr %0, pmpaddr0" : "=r"(pmpaddr0));
  if (pmpaddr0 != 0x12345)
    fail();

  pass();
  return 0;
}
