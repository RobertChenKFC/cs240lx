#include "rpi.h"

// cycle counter routines.
#include "cycle-count.h"


// this defines the period: makes it easy to keep track and share
// with the test generator.
#include "../test-gen/test-gen.h"

// trivial logging code to compute the error given a known
// period.
#include "samples.h"

// derive this experimentally: check that your pi is the same!!
#define CYCLE_PER_SEC (700*1000*1000)

// some utility routines: you don't need to use them.
#include "cycle.h"

#include "timer-interrupt.h"

#include "asm-helpers.h"
cp_asm_set(vector_base_asm, p15, 0, c12, c0, 0)
cp_asm_get(vector_base_asm, p15, 0, c12, c0, 0)

enum {
    GPIO_BASE = 0x20200000,
    gpio_fsel0 = (GPIO_BASE + 0x00),
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),

    pin = 21
};

#define read_pin(pin) (((*((volatile uint32_t *)gpio_lev0) >> pin)))
#define read_from(addr, pin) (((*((volatile uint32_t *)addr) >> pin)))

static volatile uint32_t changed = 0, gpio_cycle = 0, gpio_val;
void int_vector(uint32_t pc, uint32_t cycle_reg) {
  if (gpio_event_detected(pin)) {
    changed = 1;
    gpio_cycle = cycle_reg;
    gpio_val = read_pin(pin);
    gpio_event_clear(pin);
  }
}

// implement this code and tune it.
unsigned 
scope(register uint32_t addr, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned v1, v0 = read_from(addr, pin), v = v0;

    // spin until the pin changes.
    changed = 0;
    while (!changed);

    // when we started sampling 
    unsigned start = gpio_cycle, t = start;
    changed = 0;

    // sample until record max samples or until exceed <max_cycles>
    unsigned n = 0, iter = 0, max_iter = 1e7;
    while (n < n_max && ++iter < max_iter) {
      // write this code first: record sample when the pin
      // changes.  then start tuning the whole routine.
      if (changed) {
        l[n++] = (log_ent_t){
          .ncycles = gpio_cycle - start,
          .v = gpio_val
        };
        changed = 0;
      }
    }
    printk("timeout! start=%d, t=%d, minux=%d\n",
           start, t, cycle_cnt_read() - start);
    v &= 1;
    for (int i = 0; i < n; ++i)
      l[i].v = v ^ (i % 2);
    return n;
}

void notmain(void) {
    extern uint32_t default_vec_ints[];

    // initialie interrupts to a clean state.
    dev_barrier();
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();
    vector_base_asm_set((uint32_t)default_vec_ints);
    
    void *exp = default_vec_ints;
    void *got = (void*)vector_base_asm_get();
    if(got != exp)
        panic("expected %p, have %p\n", got, exp);

#if 1
    // set interrupts on rising and falling edges.
    gpio_int_rising_edge(pin);
    gpio_int_falling_edge(pin);
#else
    gpio_int_async_rising_edge(in_pin);
    gpio_int_async_falling_edge(in_pin);
#endif

    // clear any existing event.
    gpio_event_clear(pin);
    system_enable_interrupts();
    caches_enable();

    // setup input pin.
    gpio_set_input(pin);

    // make sure to init cycle counter hw.
    cycle_cnt_init();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    // just to illustrate.  remove this.
    // sample_ex(log, 10, CYCLE_PER_FLIP);

    volatile int addr = gpio_lev0;

    // run 4 times before rebooting: makes things easier.
    // you can get rid of this.
    for(int i = 0; i < 4; i++) {
        unsigned n = scope(addr, log, MAXSAMPLES, sec_to_cycle(1));
        dump_samples(log, n, CYCLE_PER_FLIP);
    }
    clean_reboot();
}
