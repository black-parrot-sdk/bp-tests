
#include <stdint.h>
#include <bp_utils.h>
#include <stdio.h>

// This test will fail if an fflags read or write instruction is allowed 
//   to proceed before preceeding instructions have finished
void main(uint64_t argc, char * argv[]) {
   int fflags;
   double one = 1.0;
   double zero = 0.0;
   double res;
   __asm__ __volatile__ ("fdiv.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));
   __asm__ __volatile__ ("csrr %0, fflags" : "=r"(fflags) : );
   if (fflags == 0) bp_finish(1);

   bp_finish(0);
}
