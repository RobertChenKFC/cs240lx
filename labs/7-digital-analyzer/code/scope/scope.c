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

    // DEBUG
    JTAG_TDO = 24,
    JTAG_TCK = 25,
    JTAG_TDI = 26,
    JTAG_TMS = 27,
    
    // DEBUG
    // pin = 21,
    pin = JTAG_TMS,

    led = 27
};

void print_value(uint32_t x) {
  debug("x = %d\n", x);
  clean_reboot();
}

// #define read_pin(pin) (((*((volatile uint32_t *)gpio_lev0) >> pin)))
#define read_from(addr, pin) (((*((volatile uint32_t *)addr) >> pin)))

#define MAXSAMPLES 32
static unsigned arr[MAXSAMPLES], v;
void int_handler(unsigned *ptr) {
  log_ent_t l[MAXSAMPLES];
  unsigned n = 0;
  for (unsigned *p = arr; *p; ++p, ++n) {
    l[n] = (log_ent_t){
      .ncycles = *p,
      .v = v ^ ((n + 1) % 2)
    };
  }
  // DEBUG
  // dump_samples(l, n, CYCLE_PER_FLIP);
  
  debug("ptr - arr = %d\n", ptr - arr);
  debug("n = %d, l = %x\n", n, l);
  for (int i = 0; i < MAXSAMPLES; ++i)
    debug("arr[%d] = %d\n", i, arr[i]);

  clean_reboot();
}

// implement this code and tune it.
unsigned 
scope(register uint32_t addr, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    /*
    unsigned v1, v0 = read_from(addr, pin), v = v0;

    // spin until the pin changes.
    while((v1 = read_from(addr, pin)) == v0);

    // when we started sampling 
    unsigned start = cycle_cnt_read(), t = start;

    // sample until record max samples or until exceed <max_cycles>
    unsigned n = 0, iter = 0, max_iter = 1e7;
    while (++iter < max_iter && n < n_max) {
      // write this code first: record sample when the pin
      // changes.  then start tuning the whole routine.
      v0 = read_from(addr, pin);
      if (v0 != v1) {
        l[n++].ncycles = cycle_cnt_read() - start;
        v1 = v0;
      }
    }
    printk("timeout! start=%d, t=%d, minux=%d\n",
           start, t, cycle_cnt_read() - start);
    v &= 1;
    for (int i = 0; i < n; ++i)
      l[i].v = v ^ (i % 2);
    return n;
    */
  unsigned *scope_asm(unsigned*, unsigned, unsigned);
  unsigned max_iter = 2e6;

  v = read_from(addr, pin) & 1;
  memset(arr, 0, sizeof(arr));
  unsigned *arr_end = scope_asm(arr, n_max, max_iter);
  for (unsigned *p = arr, i = 0; p != arr_end; ++p, ++i) {
    l[i] = (log_ent_t){
      .ncycles = *p,
      .v = v ^ ((i + 1) % 2)
    };
  }
  return arr_end - arr;
}

void notmain(void) {
    extern uint32_t default_vec_ints[];

    // initialize interrupts to a clean state.
    dev_barrier();
    PUT32(Disable_IRQs_1, 0xffffffff);
    PUT32(Disable_IRQs_2, 0xffffffff);
    dev_barrier();
    vector_base_asm_set((uint32_t)default_vec_ints);
    
    void *exp = default_vec_ints;
    void *got = (void*)vector_base_asm_get();
    if(got != exp)
        panic("expected %p, have %p\n", got, exp);

    system_enable_interrupts();
    caches_enable();

    // setup input pin.
    gpio_set_input(pin);

    // turn on the LED for easy identification
    gpio_set_output(led);
    gpio_write(led, 1);

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
