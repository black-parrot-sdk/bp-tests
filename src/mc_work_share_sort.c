/*
 * Name:
 *   mc_work_share_sort.c
 *
 * Description:
 *   This program utilizes many cores to cooperatively sort a collection of arrays.
 *   The backing data is defined in mc_data.h and is an array of uint32_t elements with length
 *   of DATA_LEN. This array, called DATA, is treated as a collection of uint32_t arrays that
 *   will be sorted by the cores. Each core requests an array to sort, sorts it, then repeats
 *   until all available arrays have been sorted.
 *
 */

#include <stdint.h>
#include <stddef.h>
#include "bp_utils.h"
#include "mc_util.h"
#include "mc_data.h"
#include "bp_aviary.h"

// DATA_LEN define from "mc_data.h" gives total number of elements in DATA array

// Length of each array
// This should be set such that an array evenly fills one or more cache blocks
// There are 16 32-bit values in each 512-bit cache block
// ARRAY_LEN of 32 allows 2048 arrays in default DATA array
#ifndef ARRAY_LEN
#define ARRAY_LEN 32
#endif

// Number of arrays to sort per core
// This determines the total number of arrays that will be sorted, and each core will sort
// approximately ARRAYS_PER_CORE, however the number actually sorted per core depends on
// the dynamic execution, as each core sorts arrays until there are no more arrays left to be
// sorted from the global pool.
// num_cores * ARRAYS_PER_CORE * ARRAY_LEN <= DATA_LEN is required
// ARRAY_LEN of 32 and ARRAYS_PER_CORE of 32 = 1024 array elements per core => maximum of 64 cores
#ifndef ARRAYS_PER_CORE
#define ARRAYS_PER_CORE 32
#endif

// global variables written only by core 0
volatile uint64_t __attribute__((aligned(64))) start_barrier_mem = 0;
volatile uint64_t __attribute__((aligned(64))) end_barrier_mem = 0;

// global lock for shared global variables
volatile uint64_t __attribute__((aligned(64))) global_lock = 0;

volatile uint64_t __attribute__((aligned(64))) read_count = 0;
volatile uint64_t __attribute__((aligned(64))) read_index = 0;

void swap(uint32_t *x, uint32_t * y) {
  uint32_t t = *x;
  *x = *y;
  *y = t;
}

void sortArray(uint32_t *array) {
  // bubble sort because we want the core to do work :)
  for (int i = 0; i < ARRAY_LEN-1; i++) {
    for (int j = 0; j < ARRAY_LEN-i-1; j++) {
      if (array[j] > array[j+1]) {
        swap(&array[j], &array[j+1]);
      }
    }
  }
}

uint32_t getNextArray(uint32_t **array, uint32_t total_arrays) {
  uint64_t local_read_count;
  lock(&global_lock);
  if (read_count < total_arrays) {
    local_read_count = read_count;
    *array = &DATA[read_index];
    // ARRAYS is a one-dimensional array, which we treat as a two-dimensional array
    // increment the read_index by the length of each sub-array
    read_index += ARRAY_LEN;
    read_count += 1;
    unlock(&global_lock);
    return local_read_count;
  }
  unlock(&global_lock);
  *array = NULL;
  return 0;
}

uint64_t thread_main(uint32_t total_arrays) {
  while (1) {
    uint32_t *array = NULL;
    uint32_t arr_index = getNextArray(&array, total_arrays);
    if (array) {
      sortArray(array);
    } else {
      break;
    }
  }

  // synchronize at end of computation by incrementing the end barrier
  lock(&global_lock);
  end_barrier_mem += 1;
  unlock(&global_lock);

}

uint64_t main(uint64_t argc, char * argv[]) {
  uint64_t core_id;
  __asm__ volatile("csrr %0, mhartid": "=r"(core_id): :);
  uint32_t num_cores = bp_param_get(PARAM_CC_X_DIM) * bp_param_get(PARAM_CC_Y_DIM);
  // fail if number of arrays to sort requires more data than is available
  uint32_t total_arrays = ARRAYS_PER_CORE*num_cores;
  uint32_t total_array_len = total_arrays*ARRAY_LEN;
  if (total_array_len > DATA_LEN) {
    return 1;
  }

  // only core 0 intializes data structures
  if (core_id == 0) {
    global_lock = 0;
    read_index = 0;
    read_count = 0;
    end_barrier_mem = 0;

    // signal done with initialization
    start_barrier_mem = 0xdeadbeef;
  }
  else {
    while (start_barrier_mem != 0xdeadbeef) { }
  }

  // all threads execute
  thread_main(total_arrays);

  if (core_id == 0) {
    // core 0 waits for all threads to finish
    // wait for all threads to finish
    while (end_barrier_mem != num_cores) { }
    return 0;
  } else {
    while (1) { }
  }

  // no core should reach this, return non-zero (error)
  return 1;
}

