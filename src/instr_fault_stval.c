// This test verifies that stval is written with the faulting PC
// on instruction exceptions (scause = 0 or 1) in S-mode
//
#include <stdint.h>
#include "bp_utils.h"

volatile int test_num = 0;

void s_trap_handler() {
  uint64_t scause, stval, sepc;
  __asm__ __volatile__ ("csrr %0, scause" : "=r" (scause));
  __asm__ __volatile__ ("csrr %0, stval"  : "=r" (stval));
  __asm__ __volatile__ ("csrr %0, sepc"   : "=r" (sepc));

  if (scause != 0 && scause != 1)
    bp_finish(1);

  if (test_num == 1) {
    if (stval != sepc)
      bp_finish(1);
    test_num = 2;
    uint64_t misaligned_addr = 0x80000003ULL;
    __asm__ __volatile__ ("jr %0" : : "r" (misaligned_addr));
  } else if (test_num == 2) {
    if (stval != sepc)
      bp_finish(1);
    bp_finish(0);
  } else {
    bp_finish(1);
  }
}

int main(uint64_t argc, char * argv[]) {
  uint64_t fault_addr = 0x470000000000ULL;

  // delegate ecode 0 and 1 to S-mode
  __asm__ __volatile__ ("csrw medeleg, %0" : : "r" ((1 << 0) | (1 << 1)));

  // set stvec to s_trap_handler
  __asm__ __volatile__ ("csrw stvec, %0" : : "r" (&s_trap_handler));

  // set mstatus.mpp = 01 (S-mode)
  uint64_t mstatus;
  __asm__ __volatile__ ("csrr %0, mstatus" : "=r" (mstatus));
  mstatus = (mstatus & ~(3ULL << 11)) | (1ULL << 11);
  __asm__ __volatile__ ("csrw mstatus, %0" : : "r" (mstatus));

  // disable VM (bare mode)
  __asm__ __volatile__ ("csrw satp, %0" : : "r" (0));

  // mret into S-mode at fault_addr
  test_num = 1;
  __asm__ __volatile__ ("csrw mepc, %0" : : "r" (fault_addr));
  __asm__ __volatile__ ("mret");

  // should never reach here
  bp_finish(1);
}
