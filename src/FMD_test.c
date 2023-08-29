
   #include <stdint.h>
   #include <bp_utils.h>
   #include <stdio.h>

   uint64_t num_traps = 0;

void trap_handler() {
    uint64_t return_add;

    num_traps++;

    __asm__ __volatile__ ("csrr  %0, mepc": "=r" (return_add): :);
    return_add += 4;
    __asm__ __volatile__ ("csrw mepc, %0": : "r" (return_add));
    __asm__ __volatile__ ("mret");
}

void trap_fail() {
    bp_finish(1);
}

   void main(uint64_t argc, char * argv[]) {
    int fflags;
    double one = 4.0;
    double zero = 2.0;
    double res;
    double x = 3.0;
    double y = 1.5;
    double z;
    int32_t a = 5;
    int32_t b = 3;
    int32_t c;

   __asm__ __volatile__ ("csrw mtvec, %0": : "r" (&trap_handler));
   uint64_t mstatus = 3 << 13;                                      //CLear FS bit
   __asm__ __volatile__ ("csrc mstatus, %0" : : "r" (mstatus));

    // Floating-Point (F) Extension Instructions
    __asm__ __volatile__ ("fadd.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));               //Adds two double-precision floating-point values.
    __asm__ __volatile__ ("fsub.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));               //Subtracts one double-precision floating-point value from another.
    __asm__ __volatile__ ("fmul.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));               //Multiplies two double-precision floating-point values.
    __asm__ __volatile__ ("fdiv.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));               //Divides one double-precision floating-point value by another.
    __asm__ __volatile__ ("fsqrt.d %0, %1" : "=f"(res) : "f"(one));                             //Computes the square root of a double-precision floating-point value.
    __asm__ __volatile__ ("fsgnj.d %0, %1, %2" : "=f"(res) : "f"(one), "f"(zero));              //Applies a sign operation to a double-precision floating-point value.

    // Double-Precision Floating-Point (D) Extension Instructions
    __asm__ __volatile__ ("fcvt.w.d %0, %1" : "=r"(c) : "f"(one));                              //Converts a double-precision floating-point value to a signed integer.
    __asm__ __volatile__ ("fcvt.d.w %0, %1" : "=f"(res) : "r"(c));                              //Converts a signed integer to a double-precision floating-point value.
    __asm__ __volatile__ ("feq.d %0, %1, %2" : "=r"(c) : "f"(one), "f"(zero));                  //Checks if two double-precision floating-point values are equal.
    __asm__ __volatile__ ("flt.d %0, %1, %2" : "=r"(c) : "f"(one), "f"(zero));                  //Checks if one double-precision floating-point value is less than another.
    __asm__ __volatile__ ("fle.d %0, %1, %2" : "=r"(c) : "f"(one), "f"(zero));                  //Checks if one double-precision floating-point value is less than or equal to another.
    __asm__ __volatile__ ("fcvt.d.l %0, %1" : "=f"(res) : "r"(c));                              //Converts a signed integer to a double-precision floating-point value.


      bp_finish(0);
   }