
#include <stdint.h>
#include <bp_utils.h>
#include "mc_util.h"

// N must be multiple of 100
#define N 500

#define LOOP_INSTR \
{                  \
  __asm__ __volatile__ ("amoadd.d x0, a5, (a4)"); \
}

#include "mc_asm_loop.h"

volatile uint64_t __attribute__((aligned(64))) global_lock = 0;
volatile uint64_t __attribute__((aligned(64))) end_barrier_mem = 0;
uint64_t amo_target __attribute__ ((aligned (16))) = 0;

uint64_t main(uint64_t argc, char * argv[]) {

  uint64_t core_id;
  __asm__ volatile("csrr %0, mhartid": "=r"(core_id): :);
  uint32_t num_cores = bp_param_get(PARAM_CC_X_DIM) * bp_param_get(PARAM_CC_Y_DIM);

  uint64_t* amo_target_addr = &amo_target;
  int iters = N/100;
  LOOP;

  lock(&global_lock);
  end_barrier_mem += 1;
  unlock(&global_lock);

  if (core_id == 0) {
    // core 0 waits for all threads to finish
    // wait for all threads to finish
    while (end_barrier_mem != num_cores) { }
    uint64_t total = num_cores * N;
    return (amo_target != total);
  } else {
    bp_finish(0);
    while (1) { }
  }

  // no core should reach this, return non-zero (error)
  return 1;
}
