#include <stdio.h>
#include <stdint.h>
#include "bp_utils.h"
#include "bp_asm.h"

#define NUM_TEST_ITER 1
#define DEBUG

#include "tc.h"
#include "tc_1.h"

void check() {
    uint64_t errors = 0;
    for (int i = 0; i < I*K; i++) {
        if (R[i] != E[i]) {
            errors++;
            #ifdef DEBUG
                bp_printf("%d: %d != %d\n", i, R[i], E[i]);
            #endif
        }
    }
    bp_printf("Number of errors = %ld\n", errors);
    if (errors > 0) {
        bp_print_string("\n\tERRORS DETECTED!!!\n\n");
    } else {
        bp_print_string("\n\tCorrect\n\n");
    }
}

void check_blk() {
    uint64_t errors = 0;
    for (int i = 0; i < I*K*(J/4); i++) {
        if (R_BLK[i] != E_BLK[i]) {
            errors++;
            #ifdef DEBUG
                bp_printf("%d: %d != %d\n", i, R[i], E[i]);
            #endif
        }
    }
    bp_printf("Number of errors = %ld\n", errors);
    if (errors > 0) {
        bp_print_string("\n\tERRORS DETECTED!!!\n\n");
    } else {
        bp_print_string("\n\tCorrect\n\n");
    }
}

void reset_gemm()
{
    for (int q = 0; q < I*K*(J/4); q++) { R_BLK[q] = 0; }
}

int main(int argc, char** argv)
{
    for (int t = 0; t < NUM_TEST_ITER; t++)
    {
        //reset_gemm();
        nuke_l2();

        uint64_t A_base = (uint64_t)(&A[0]);
        uint64_t W_base = (uint64_t)(&W[0]);
        uint64_t R_base = (uint64_t)(&R_BLK[0]);

        bp_printf("A_base = %ld\n", A_base);
        bp_printf("W_base = %ld\n", W_base);
        bp_printf("R_base = %ld\n", R_base);

        bool buf = 0;

        //A_base -= 64;
        //W_base -= 64;

        const int IBLK = I / 4;
        const int JBLK = J / 4;
        const int KBLK = K / 4;

        uint64_t start_cycles = get_cycle();

        for (uint64_t j = 0; j < JBLK; j++) {
        for (uint64_t k = 0; k < KBLK; k++) {

            uint64_t W_offset = (j + k*JBLK) * 64;
            uint64_t W_ptr = W_base + W_offset;

            buf ? tensor_wl(W_ptr, 1, 0)
                : tensor_wl(W_ptr, 0, 0)
                ;

            for (uint64_t i = 0; i < IBLK; i+=1) {

                uint64_t R_offset = (i + k*IBLK + j*KBLK*IBLK) * 64;
                uint64_t R_ptr = R_base + R_offset;

                tensor_csr_st(0, R_ptr);

                uint64_t A_offset = (i + j*IBLK) * 64;
                uint64_t A_ptr = A_base + A_offset;

                buf ? tensor_al(A_ptr, 1, 0)
                    : tensor_al(A_ptr, 0, 0)
                    ;

            }
            buf = !buf;
        }}

        uint64_t end_cycles = get_cycle();
        uint64_t diff = end_cycles - start_cycles;
        bp_printf("Cycle Count = %lu\n", diff);

        nuke_l2();
        check_blk();

        float flops = (float)(I*J*K) / diff;
        bp_printf("FLOPS = %f\n", flops);
    }

    bp_printf("WL_STALLS = %ld\n", tensor_csr_ld(2));
    bp_printf("AL_STALLS = %ld\n", tensor_csr_ld(3));
    bp_printf("PANIC_STALLS = %ld\n", tensor_csr_ld(4));
    return 0;
}