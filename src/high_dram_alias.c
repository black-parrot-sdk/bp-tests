
#include <stdint.h>
#include <bp_utils.h>

void main(uint64_t argc, char *argv[]) {
  volatile uint64_t *low_addr  = (volatile uint64_t *)0x083000000ULL;
  volatile uint64_t *high_addr = (volatile uint64_t *)0x183000000ULL;

  uint64_t pattern_low   = 0x1122334455667788ULL;
  uint64_t pattern_high  = 0xAABBCCDDEEFF0011ULL;
  uint64_t pattern_low2  = 0x55AA55AA55AA55AAULL;
  uint64_t pattern_high2 = 0xCAFEBABECAFED00DULL;

  *low_addr = pattern_low;
  if (*low_addr != pattern_low)
    bp_finish(1);

  *high_addr = pattern_high;
  if (*high_addr != pattern_high)
    bp_finish(1);

  if (*low_addr != pattern_low)
    bp_finish(1);

  *low_addr = pattern_low2;
  if (*low_addr != pattern_low2)
    bp_finish(1);

  if (*high_addr != pattern_high)
    bp_finish(1);

  *high_addr = pattern_high2;
  if (*high_addr != pattern_high2)
    bp_finish(1);

  if (*low_addr != pattern_low2)
    bp_finish(1);

  bp_finish(0);
}
