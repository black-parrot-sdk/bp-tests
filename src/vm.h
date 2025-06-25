#ifndef VM_H
#define VM_H

#define CAUSE_MISALIGNED_FETCH    0x0
#define CAUSE_FETCH_ACCESS        0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT          0x3
#define CAUSE_MISALIGNED_LOAD     0x4
#define CAUSE_LOAD_ACCESS         0x5
#define CAUSE_MISALIGNED_STORE    0x6
#define CAUSE_STORE_ACCESS        0x7
#define CAUSE_USER_ECALL          0x8
#define CAUSE_SUPERVISOR_ECALL    0x9
#define CAUSE_HYPERVISOR_ECALL    0xa
#define CAUSE_MACHINE_ECALL       0xb
#define CAUSE_FETCH_PAGE_FAULT    0xc
#define CAUSE_LOAD_PAGE_FAULT     0xd
#define CAUSE_STORE_PAGE_FAULT    0xf

#define PTE_S_LEAF (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)
#define PTE_U_LEAF (PTE_V | PTE_U | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)

#define PGSHIFT       12
#define PTE_PPN_SHIFT 10
#define PGLEVELBITS   9
#define PTES_PER_PT   (1UL << PGLEVELBITS)
#define PGSIZE        (1UL << PGSHIFT)
#define MPGSIZE       (1UL << (PGSHIFT + PGLEVELBITS))
#define GPGSIZE       (1UL << (PGSHIFT + 2*PGLEVELBITS))

#define DRAM_BASE      0x80000000

#define vpn0(va) (((va) >> PGSHIFT) % PTES_PER_PT)
#define vpn1(va) (((va) >> (PGSHIFT + PGLEVELBITS)) % PTES_PER_PT)
#define vpn2(va) (((va) >> (PGSHIFT + 2*PGLEVELBITS)) % PTES_PER_PT)

#define flush_tlb() asm volatile ("sfence.vma")
#define flush_icache() asm volatile ("fence.i")

// Syscall interface, trap handled in vm_start.S. a0 is argument to call, a1 is
// syscall number. Syscall 0 is bp_finish.
#define terminate(code)             \
  asm volatile("li a1, 0\n\t"       \
               "mv a0, %0\n\t"      \
               "ecall\n\t"          \
               ::"r" ((int)(code))  \
               :"%a0", "%a1")

typedef struct
{
  long gpr[32];
  long sr;
  long epc;
  long badvaddr;
  long cause;
  long hartid;
} trapframe_t;

void* memset(void*, int, size_t);

#endif

