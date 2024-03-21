
WITH_MARCH ?= rv64gc
WITH_MABI ?= lp64d

include Makefile.frag

RISCV_GCC           = $(CROSS_COMPILE)gcc
RISCV_GPP           = $(CROSS_COMPILE)g++
RISCV_GCC_OPTS      = -march=$(WITH_MARCH) -mabi=$(WITH_MABI) -mcmodel=medany -I$(BP_SDK_INCLUDE_DIR)
RISCV_LINK_OPTS     = -T$(BP_SDK_LINKER_DIR)/riscv.ld -L$(BP_SDK_LIB_DIR) -Wl,--whole-archive -lperchbm -Wl,--no-whole-archive

.PHONY: all

vpath %.c   ./src
vpath %.cpp ./src
vpath %.S   ./src

all: $(foreach x,$(subst -,_,$(BP_TESTS)),$(x).riscv)

%.riscv: %.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

%.riscv: %.S
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

%.riscv: %.cpp
	$(RISCV_GPP) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

paging.riscv: vm_start.S paging.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS) -nostartfiles

mapping.riscv: vm_start.S mapping.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS) -nostartfiles

streaming_accelerator_loopback.riscv: streaming_accelerator_loopback.c vdp.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

streaming_accelerator_vdp.riscv: streaming_accelerator_vdp.c vdp.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

streaming_accelerator_zipline.riscv: streaming_accelerator_zipline.c vdp.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

coherent_accelerator_vdp.riscv: coherent_accelerator_vdp.c vdp.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS)

misaligned_instructions_virtual_memory.riscv: vm_start.S misaligned_instructions_virtual_memory.c
	$(RISCV_GCC) -o $@ $^ $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS) -nostartfiles

clean:
	rm -f *.riscv

