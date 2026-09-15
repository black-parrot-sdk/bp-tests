BP_TESTS_C = \
  out_of_bounds_test    \
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
  interrupt_priority    \
  interrupt_priority_simultaneous \
  interrupt_priority_race \
  instr_fault_mtval     \
  instr_fault_stval     \
  instret_smode         \
  instret_smode_deny    \
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
  high_dram_alias       \
  misaligned_ldst       \
  nanboxing             \
  misaligned_instructions_basic_jumps \
  misaligned_instructions_advanced_jumps \
  misaligned_instructions_virtual_memory \
  execute_dynamic_instruction \
  fp_precision          \
  fp_neg_zero_nanbox    \
  fp_signed_zero        \
  pmp_csr_enabled       \
  pmp_csr_disabled_illegal \
  satp_u_mode           \
  satp_supported_modes  \
  pmp_s_mode            \
  mprv_u_page           \
  mprv_debug_page       \
  cycle_write           \
  gigapage_alignment    \
  pte_reserved          \
  pte_valid_nonleaf     \
  counteren_u_mode      \
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
