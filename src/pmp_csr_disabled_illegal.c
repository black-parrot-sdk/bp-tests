#include <stdint.h>
#include <bp_utils.h>

volatile uint64_t illegal_count;

static inline void pass(void) { bp_finish(0); }
static inline void fail(void) { bp_finish(1); }

void trap_continue(void)
{
  uint64_t mcause;
  uint64_t mepc;
  uint16_t instr16;
  uint64_t next_pc;

  __asm__ __volatile__("csrr %0, mcause" : "=r"(mcause));
  if (mcause != 2)
    fail();

  __asm__ __volatile__("csrr %0, mepc" : "=r"(mepc));
  instr16 = *(volatile uint16_t *)mepc;
  next_pc = mepc + (((instr16 & 0x3) == 0x3) ? 4 : 2);
  __asm__ __volatile__("csrw mepc, %0" : : "r"(next_pc));

  illegal_count++;
  __asm__ __volatile__("mret");
}

int main(uint64_t argc, char *argv[])
{
  uint64_t tmp;

  illegal_count = 0;
  __asm__ __volatile__("csrw mtvec, %0" : : "r"(&trap_continue));

  // Both accesses should trap as illegal when num_pmp_entries=0.
  __asm__ __volatile__("csrr %0, pmpcfg0" : "=r"(tmp));
  __asm__ __volatile__("csrw pmpaddr0, %0" : : "r"(tmp));

  if (illegal_count != 2)
    fail();

  pass();
  return 0;
}
