#ifndef __ARMV6_PMU_H__
#define __ARMV6_PMU_H__
#include "bit-support.h"
#include "asm-helpers.h"

enum {
    PMU_BRANCH_MISPREDICT = 0x6,
    PMU_BRANCH = 0x5,
    PMU_ICACHE_MISS = 0x0,
    PMU_DCACHE_MISS = 0xB,
    PMU_CALL = 0x23,
    PMU_PC_CHANGED = 0xd,
};

// implement these.

// get the type of event0 by reading the type
// field from the PMU control register and 
// returning it.
// ch3, p.133, Section 3.2.51
cp_asm(pmu_ctrl_reg, p15, 0, c15, c12, 0)
static inline uint32_t pmu_type0(void) {
  // ch3, p.134, Table 3-136
  return (pmu_ctrl_reg_get() >> 20) & 0xff;
}

// set PMU event0 as <type> and enable it.
// ch3, p.138, Section 3.2.53
cp_asm(pmu_cnt0, p15, 0, c15, c12, 2)
static inline void pmu_enable0(uint32_t type) {
  // ch3, p.134, Table 3-136
  uint32_t x = pmu_ctrl_reg_get();
  //    EvtCount0
  x |= type << 20;
  pmu_ctrl_reg_set(x);
  pmu_cnt0_set(0);
  assert(pmu_type0() == type);
}

// get current value for event 0 
static inline uint32_t pmu_event0_get(void) {
  return pmu_cnt0_get();
}

// get the type of event1 by reading the type
// field from the PMU control register and 
// returning it.
static inline uint32_t pmu_type1(void) {
  // ch3, p.134, Table 3-136
  return (pmu_ctrl_reg_get() >> 12) & 0xff;
}

// set event1 as <type> and enable it.
// ch3, p.139, Section 3.2.54
cp_asm(pmu_cnt1, p15, 0, c15, c12, 3)
static inline void pmu_enable1(uint32_t type) {
  assert((type & 0xff) == type);
  // ch3, p.134, Table 3-136
  uint32_t x = pmu_ctrl_reg_get();

  //    EvtCount1
  x |= type << 12;
  pmu_ctrl_reg_set(x);
  pmu_cnt1_set(0);
  assert(pmu_type1() == type);
}

static inline uint32_t pmu_event1_get(void) {
  return pmu_cnt1_get();
}

// wrapper so can pass in the PMU register number.
static inline void pmu_enable(unsigned n, uint32_t type) {
    if(n==0)
        pmu_enable0(type);
    else if(n == 1)
        pmu_enable1(type);
    else
        panic("bad PMU coprocessor number=%d\n", n);
}

// wrapper so can pass in the PMU register number.
static inline uint32_t pmu_event_get(unsigned n) {
    if(n==0)
        return pmu_event0_get();
    else if(n == 1)
        return pmu_event1_get();
    else
        panic("bad PMU coprocessor number=%d\n", n);
}
#endif
