
#include <stdint.h>
#include "bp_utils.h"

// RISC-V Privileged ISA v1.12 requires scause Exception Code
// to preserve values 0 through 31 (bits 4:0).

static uint64_t scause_roundtrip(uint64_t value) {
    uint64_t readback;

    __asm__ __volatile__ (
        "csrw scause, %1\n\t"
        "csrr %0, scause"
        : "=r" (readback)
        : "r" (value)
    );

    return readback;
}

int main(uint64_t argc, char *argv[]) {
    for (uint64_t value = 0; value <= 31; value++) {
        if (scause_roundtrip(value) != value) {
            bp_finish(1);
        }
    }

    return 0;
}
