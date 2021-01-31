#include <string.h>
#include "bp_utils.h"
#include "vm.h"

#define NPT 4
#define l1pt pt[0]
#define l2pt pt[1]
#define l3pt pt[2]
#define kpt  pt[3]

#define pa2kva(pa) ((uint64_t)(pa) - DRAM_BASE - MPGSIZE)
#define kva2pa(va) ((uint64_t)(va) + DRAM_BASE + MPGSIZE)

void trap_entry();
void pop_tf(trapframe_t*);

uint64_t pt[NPT][PTES_PER_PT] __attribute__((aligned(PGSIZE)));

static void page_init(uint64_t addr) {
  // Virtual 4k pages are mapped to the vpn0'th page of the second megapage
  addr = (addr/PGSIZE)*PGSIZE;
  uint64_t page_pa = DRAM_BASE + MPGSIZE + (PGSIZE * vpn0(addr));
  l1pt[vpn2(addr)] = ((uint64_t)kva2pa(l2pt) >> PGSHIFT << PTE_PPN_SHIFT) | PTE_V;
  l2pt[vpn1(addr)] = ((uint64_t)kva2pa(l3pt) >> PGSHIFT << PTE_PPN_SHIFT) | PTE_V;
  l3pt[vpn0(addr)] = ((uint64_t)page_pa >> PGSHIFT << PTE_PPN_SHIFT) | PTE_U_LEAF; 
}

static void megapage_init(uint64_t addr) {
  // Virtual megapages are mapped to the vpn1'th megapage
  addr = (addr/MPGSIZE)*MPGSIZE;
  uint64_t page_pa = DRAM_BASE + (MPGSIZE * vpn1(addr));
  l1pt[vpn2(addr)] = ((uint64_t)kva2pa(l2pt) >> PGSHIFT << PTE_PPN_SHIFT) | PTE_V;
  l2pt[vpn1(addr)] = ((uint64_t)page_pa >> PGSHIFT << PTE_PPN_SHIFT) | PTE_U_LEAF;
}

static void gigapage_init(uint64_t addr) {
  // All virtual gigapage are mapped to the first gigapage
  addr = (addr/GPGSIZE)*GPGSIZE;
  uint64_t page_pa = DRAM_BASE;
  l1pt[vpn2(addr)] = ((uint64_t)page_pa >> PGSHIFT << PTE_PPN_SHIFT) | PTE_U_LEAF;
}

void handle_fault(uint64_t addr, int cause) {
  if(!((vpn2(addr) == vpn1(addr)) && (vpn1(addr) == vpn0(addr))))
    terminate(-1);
  if(vpn2(addr) % 3 == 0)
    gigapage_init(addr);
  else if(vpn1(addr) % 3 == 1)
    megapage_init(addr);
  else
    page_init(addr);
  flush_tlb();
}

void handle_trap(trapframe_t* tf) {
  if (tf->cause == CAUSE_FETCH_PAGE_FAULT || tf->cause == CAUSE_LOAD_PAGE_FAULT || tf->cause == CAUSE_STORE_PAGE_FAULT)
    handle_fault(tf->badvaddr, tf->cause);
  else
    terminate(-1);

  pop_tf(tf);
}
void userTest() {
  for(int i=1; i < 40; i++) {
    *(uint64_t*)(i * (GPGSIZE + MPGSIZE + PGSIZE)) = i;
  }
  for(int i=1; i < 40; i++) {
    uint64_t data = *(uint64_t*)(i * (GPGSIZE + MPGSIZE + PGSIZE));
    if(data != i)
      terminate(-1);
  }
  terminate(0);
}

int main(int argc, char** argv) {

  uint64_t satp_val = ((uint64_t)l1pt >> PGSHIFT) | ((uint64_t)SATP_MODE_SV39 * (SATP_MODE & ~(SATP_MODE<<1)));
  write_csr(satp, satp_val);
  set_csr(mstatus, MSTATUS_SUM);
  write_csr(stvec, pa2kva(trap_entry));
  write_csr(sscratch, pa2kva(read_csr(mscratch)));
  write_csr(medeleg,
    (1 << CAUSE_FETCH_PAGE_FAULT) |
    (1 << CAUSE_LOAD_PAGE_FAULT) |
    (1 << CAUSE_STORE_PAGE_FAULT));

  // Allocate the last megapage to kernel
  l1pt[PTES_PER_PT-1] = ((uint64_t)kpt >> PGSHIFT << PTE_PPN_SHIFT) | PTE_V;
  kpt[PTES_PER_PT-1] = ((uint64_t)DRAM_BASE >> PGSHIFT << PTE_PPN_SHIFT) | PTE_S_LEAF;

  trapframe_t tf;
  memset(&tf, 0, sizeof(tf));
  tf.epc = (uint64_t)userTest - DRAM_BASE;
  tf.gpr[2] = PGSIZE - 0x100;
  pop_tf(&tf);

  return 0;  
}
