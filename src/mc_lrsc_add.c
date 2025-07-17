
#include <stdint.h>
#include <bp_utils.h>
#include "mc_util.h"

// number of iterations for each core to run the main loop
#define N 500

volatile uint64_t __attribute__((aligned(64))) amo_target = 0;
volatile uint64_t __attribute__((aligned(64))) global_lock = 0;
volatile uint64_t __attribute__((aligned(64))) start_barrier_mem = 0;
volatile uint64_t __attribute__((aligned(64))) end_barrier_mem = 0;

void thread_main(void) {
  int iters = N;
  uint64_t sc_result = 0;
  uint64_t sc_load = 0;
  int i = 0;

  // main loop
  // Execute LR/SC that performs amo_add.d of 1 to amo_target
  __asm__ __volatile__ (
    "j .Lcheck%=;"
    ".Llrsc%=:;"
    "lr.d %[sc_load], (%[amo_addr]);" // load to sc_load
    "addi %[sc_result],%[sc_load],1;" // sc_result = sc_load + 1
    "sc.d %[sc_result],%[sc_result],(%[amo_addr]);" // sc_result = store sc_result to amo_target_addr
    "bnez %[sc_result], .Llrsc%=;" // bnez sc_result
    ".Lcheck%=:;"
    "addi %[i], %[i], 1;" // i = i + 1
    "bge %[iters], %[i], .Llrsc%=;" // bge N, i
    : [sc_result] "+r" (sc_result), [sc_load] "+r" (sc_load), [i] "+r" (i)
    : [amo_addr] "r" (&amo_target), [iters] "r" (iters)
    :
    );
  lock(&global_lock);
  end_barrier_mem += 1;
  unlock(&global_lock);
}

uint64_t main(uint64_t argc, char * argv[]) {
  uint64_t core_id;
  __asm__ volatile("csrr %0, mhartid": "=r"(core_id): :);
  uint32_t num_cores = bp_param_get(PARAM_CC_X_DIM) * bp_param_get(PARAM_CC_Y_DIM);

  // only core 0 intializes data structures
  if (core_id == 0) {
    global_lock = 0;

	amo_target = 0;

    // signal done with initialization
    start_barrier_mem = 0xdeadbeef;
  }
  else {
    while (start_barrier_mem != 0xdeadbeef) { }
  }

  thread_main();

  if (core_id == 0) {
    // core 0 waits for all threads to finish
    // wait for all threads to finish
    while (end_barrier_mem != num_cores) { }
    return 0;
  } else {
    // all other cores call finish (0 = pass, 1 = fail) ...
    // ... then busy-wait for core 0 to terminate the execution
    while (1) { }
  }

  // no core should reach this, return non-zero (error)
  return 1;
}

