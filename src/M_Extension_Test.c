#include <stdint.h>
#include <bp_utils.h>
#include <stdio.h>

int main() {
    int a = 7;
    int b = 3;
    int result;

    // Test multiplication (MUL)
    __asm__ __volatile__ ("mul %0, %1, %2" : "=r"(result) : "r"(a), "r"(b));
    printf("Multiplication result: %d\n", result);

    // Test division (DIV)
    __asm__ __volatile__ ("div %0, %1, %2" : "=r"(result) : "r"(a), "r"(b));
    printf("Division result: %d\n", result);

    // Test remainder (REM)
    __asm__ __volatile__ ("rem %0, %1, %2" : "=r"(result) : "r"(a), "r"(b));
    printf("Remainder result: %d\n", result);

    return 0;
}