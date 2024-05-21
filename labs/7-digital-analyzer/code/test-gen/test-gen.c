// simple example test generator: a lot of error.  you should reduce it.
#include "rpi.h"
#include "test-gen.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "armv6-pmu.h"

enum {
    GPIO_BASE = 0x20200000,
    gpio_fsel0 = (GPIO_BASE + 0x00),
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),

    pin = 21
};
#define set_addr(addr) (*((volatile uint32_t *)addr) = (1 << pin));
#define set_val(addr, val) (*((volatile uint32_t *)addr) = (val));

static inline uint32_t cycle_cnt_read_aligned() {
  uint32_t out;
  asm volatile(".align 4\n\tMRC p15, 0, %0, c15, c12, 1" : "=r"(out));
  return out;
}

#define loop_body(cur_addr) \
  do { \
    set_val(cur_addr, val); \
    ndelay += ncycle; \
    /* we are not sure: are we delaying too much or too little? */ \
    /* delay_ncycles(start, ndelay); */ \
    while (cycle_cnt_read() <= ndelay); \
  } while (0)

// DEBUG
void debug_function(uint32_t addr) {
  debug("Hey, I am called with value %x\n", addr);
}

// send N samples at <ncycle> cycles each in a simple way.
// a bunch of error sources here.
void test_gen(
    register unsigned addr_set, register unsigned addr_clr,
    register unsigned val, register unsigned ncycle) {
  unsigned ndelay = 0;
  unsigned N = 11;
  unsigned start = cycle_cnt_read();
  ndelay += start;

  unsigned v = 1;
#if 1
  // PMU stuff
  // pmu_enable0(PMU_BRANCH_MISPREDICT);
  // pmu_enable1(PMU_BRANCH);

  void test_gen_asm(void);
  test_gen_asm();

  // debug("branch mispredicts: %d\n", pmu_event0_get());
  // debug("branch: %d\n", pmu_event1_get());

#endif
#if 0
  for(unsigned i = 0; i < 5; ++i) {
    loop_body(addr_set);
    loop_body(addr_clr);
  }
  loop_body(addr_set);
#endif
#if 0
  for (unsigned i = 0; i < 11; ++i)
    loop_body;
#endif
#if 0
  loop_body(addr_set);
  loop_body(addr_clr);
  loop_body(addr_set);
  loop_body(addr_clr);
  loop_body(addr_set);
  loop_body(addr_clr);
  loop_body(addr_set);
  loop_body(addr_clr);
  loop_body(addr_set);
  loop_body(addr_clr);
  loop_body(addr_set);
#endif

  unsigned end = cycle_cnt_read();
  printk("expected %d cycles, have %d, v=%d\n", CYCLE_PER_FLIP*N, end-start,v);
}

void notmain(void) {
    caches_enable();

#if 1
    uint32_t test_delay_loop(void);
    debug("Took %d cycles\n", test_delay_loop());
    debug("Took %d cycles\n", test_delay_loop());
    debug("Took %d cycles\n", test_delay_loop());
    debug("Took %d cycles\n", test_delay_loop());
    debug("Took %d cycles\n", test_delay_loop());
#endif

    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen(gpio_set0, gpio_clr0, 1 << pin, CYCLE_PER_FLIP - 1);

    clean_reboot();
}
