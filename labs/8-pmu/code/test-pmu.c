#include "rpi.h"
#include "armv6-pmu.h"
#include "mini-step.h"
#include "pt-vm.h"
#include "cycle-util.h"

enum {
    PMU_INST_CNT = 0x7,

    OneMB = 1024 * 1024,
    cache_addr = OneMB * 8
};

void f(void* x) {
  void *p = (void*)cache_addr;

  *((volatile uint32_t *)p);
  *((volatile uint32_t *)p);
  *((volatile uint32_t *)p);
  *((volatile uint32_t *)p);
  *((volatile uint32_t *)p);

  *((volatile uint32_t *)(p + 1000));
  *((volatile uint32_t *)(p + 1000));
  *((volatile uint32_t *)(p + 1000));
  *((volatile uint32_t *)(p + 1000));
  *((volatile uint32_t *)(p + 1000));

  *((volatile uint32_t *)(p + 2000));
}

void g(void *x) {
  f(x);
  f(x);
  f(x);
}

void step_handler(void *data, step_fault_t *fault) {
  uint32_t prev_cnt, cnt;
  asm volatile("mrc p15, 0, %0, c13, c0, 3" : "=r" (prev_cnt));
  asm volatile("mrc p15, 0, %0, c13, c0, 2" : "=r" (cnt));
  debug("Count: %d (cur: %d, prev: %d)\n", cnt - prev_cnt, cnt, prev_cnt);
}

void measure_dcache(const char *msg) {
    volatile uint32_t *ptr = (void*)cache_addr;

    // DEBUG
    unsigned cycle1 = cycle_cnt_read();
    // unsigned cycle1 = pmu_event1_get();
    // unsigned miss0  = pmu_event0_get();
    for(int i = 0; i < 10; i++) {
        *ptr; *ptr; *ptr; *ptr; *ptr;
        *ptr; *ptr; *ptr; *ptr; *ptr;
        *ptr; *ptr; *ptr; *ptr; *ptr;
        *ptr; *ptr; *ptr; *ptr; *ptr;
    }
    // DEBUG
    // unsigned miss0_end  = pmu_event0_get();
    // unsigned cycle1_end = pmu_event1_get();
    unsigned cycle1_end = cycle_cnt_read();
    // output("\t%s: total %s=%d, total %s=%d\n",
    output("\t%s: total %s=%d\n",
        msg,
        // "misses", miss0_end - miss0,
        "cycles", cycle1_end - cycle1);
}

void enable_mmu(void) {
  kmalloc_init_set_start((void*)OneMB, OneMB);

  vm_pt_t *pt = vm_pt_alloc(PT_LEVEL1_N);
  vm_mmu_init(dom_no_access);

  enum { 
      dom_kern = 1, // domain id for kernel
  };          
  pin_t dev = pin_mk_global(dom_kern, perm_rw_user, MEM_device);
  vm_map_sec(pt, 0x20000000, 0x20000000, dev);
  vm_map_sec(pt, 0x20100000, 0x20100000, dev);
  vm_map_sec(pt, 0x20200000, 0x20200000, dev);
  pin_t kern = pin_mk_global(dom_kern, perm_rw_user, MEM_uncached);
  vm_map_sec(pt, 0, 0, kern);              
  pin_t cached = pin_mk_global(dom_kern, perm_rw_user, MEM_cache_wb_alloc);
  vm_map_sec(pt, OneMB, OneMB, kern);     
  vm_map_sec(pt, cache_addr, cache_addr, cached);     
  uint32_t kern_stack = STACK_ADDR-OneMB;
  vm_map_sec(pt, kern_stack, kern_stack, kern);
  vm_map_sec(pt, kern_stack-OneMB, kern_stack-OneMB, kern);
  uint32_t except_stack = INT_STACK_ADDR-OneMB;
  vm_map_sec(pt, except_stack, except_stack, kern);
  vm_map_sec(pt, except_stack-OneMB, except_stack-OneMB, kern);
  domain_access_ctrl_set(DOM_client << dom_kern*2); 

  enum { ASID = 1, PID = 128 };
  mmu_set_ctx(PID, ASID, pt);

  mmu_enable();
  assert(mmu_is_enabled());

  // DEBUG
  output("MMU enabled!\n");

  /*
  full_except_install(0);
  full_except_set_data_abort(fault_handler);
  full_except_set_prefetch(prefetch_handler);
  */
}

void enable_dcache() {
  cp15_ctrl_reg1_t reg = cp15_ctrl_reg1_rd();
  assert(!reg.C_unified_enable);
  reg.C_unified_enable = 1;
  cp15_ctrl_reg1_wr(reg);
  reg = cp15_ctrl_reg1_rd();
  assert(reg.C_unified_enable);
}

void enable_icache() {
  cp15_ctrl_reg1_t reg = cp15_ctrl_reg1_rd();
  assert(!reg.I_icache_enable);
  reg.I_icache_enable = 1;
  cp15_ctrl_reg1_wr(reg);
  reg = cp15_ctrl_reg1_rd();
  assert(reg.I_icache_enable);
}

void enable_branch_predictor() {
  cp15_ctrl_reg1_t reg = cp15_ctrl_reg1_rd();
  assert(!reg.Z_branch_pred);
  reg.Z_branch_pred = 1;
  cp15_ctrl_reg1_wr(reg);
  reg = cp15_ctrl_reg1_rd();
  assert(reg.Z_branch_pred);
}

void notmain(void) {
    pmu_enable0(PMU_INST_CNT);
    assert(pmu_type0() == PMU_INST_CNT);

    uint32_t inst_s = pmu_event0_get();
    uint32_t inst_e = pmu_event0_get();
    output("empty instructions= %d\n", inst_e - inst_s);

    inst_s = pmu_event0_get();
    asm volatile("nop"); // 1
    asm volatile("nop"); // 2
    asm volatile("nop"); // 3
    asm volatile("nop"); // 4
    asm volatile("nop"); // 5

    asm volatile("nop"); // 1
    asm volatile("nop"); // 2
    asm volatile("nop"); // 3
    asm volatile("nop"); // 4
    asm volatile("nop"); // 5
    inst_e = pmu_event0_get();
    output("instructions= %d\n", inst_e - inst_s);

    inst_s = pmu_event0_get();
    asm volatile("nop"); // 1
    inst_e = pmu_event0_get();
    output("instructions= %d\n", inst_e - inst_s);

    inst_s = pmu_event0_get();
    asm volatile("nop"); // 1
    inst_e = pmu_event0_get();
    output("instructions= %d\n", inst_e - inst_s);

    // DEBUG
    // measure_dcache("before caches are enabled");

    enable_mmu();
    enable_dcache();
    enable_icache();
    enable_branch_predictor();

    // DEBUG
    // measure_dcache("after caches are enabled");
    uint32_t a, b;
    // pmu_enable1(PMU_INST_CNT);

#if 0
    pmu_enable1(PMU_DCACHE_MISS);
    
    a = pmu_event1_get();
    f((void*)(cache_addr));
    b = pmu_event1_get();
    debug("miss: %d\n", b - a);

    mini_step_init(step_handler, NULL);
    a = pmu_event1_get();
    mini_step_run(f, (void*)cache_addr + 4000);
    b = pmu_event1_get();
    debug("miss: %d\n", b - a);
#endif
    pmu_enable1(PMU_CALL);
    
    a = pmu_event1_get();
    g(NULL);
    b = pmu_event1_get();
    debug("calls: %d\n", b - a);

    mini_step_init(step_handler, NULL);
    a = pmu_event1_get();
    mini_step_run(g, NULL);
    b = pmu_event1_get();
    debug("calls: %d\n", b - a);
}
