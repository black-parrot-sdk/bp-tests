BP_TESTS_C = \
  m_external_interrupt  \
  s_external_interrupt  \
  hello_world           \
  instr_coherence       \
  cache_hammer          \
  jalr_illegal          \
  satp_nofence          \
  timer_interrupt       \
  loop                  \
  cache_flush           \
  stream_hammer         \
  domain_fault          \
  eaddr_fault           \
  paging                \
  mapping               \
  mstatus_fs            \
  wfi                   \
  uncached_mode         \
  amo_nonblocking       \
  amo_interrupt         \
  divide_hazard         \
  unhandled_trap        \
  aviary_rom            \
  readonly              \
  epc                   \
  fflags_haz            \
  dram_stress           \
  misaligned_ldst       \
  nanboxing             \
  FMD_test              \
  M_Extension_Test      \
  misaligned_instructions_basic_jumps \
  misaligned_instructions_advanced_jumps \
  misaligned_instructions_virtual_memory \
  execute_dynamic_instruction \
  fp_precision          \
  fp_neg_zero_nanbox    \
  fp_signed_zero        \
  l2_uncached           \
  l2_cache_ops

BP_MULTICORE_TESTS_C = \
  mc_sanity             \
  mc_template           \
  mc_rand_walk          \
  mc_work_share_sort    \
  mc_amo_add            \
  mc_lrsc_add           \

BP_ACCELERATOR_TESTS_C = \
  streaming_accelerator_vdp      \
  streaming_accelerator_loopback \
  streaming_accelerator_zipline  \
  coherent_accelerator_vdp       \

BP_TESTS_CPP = \
  virtual               \
  constructor           \
  template              \
  unwinding             \
  vector                \
  map                   \

BP_TESTS = $(BP_TESTS_C) $(BP_MULTICORE_TESTS_C) $(BP_ACCELERATOR_TESTS_C) $(BP_TESTS_CPP)

