#include "rpi.h"
#include "cycle-util.h"
#include "jtag.h"

enum {
  CLK_CYCLES = 700, // 1MHz
  CLK_CYCLES_HALF = CLK_CYCLES / 2,

  GPIO_BASE = 0x20200000,
  gpio_fsel0 = (GPIO_BASE + 0x00),
  gpio_set0  = (GPIO_BASE + 0x1C),
  gpio_clr0  = (GPIO_BASE + 0x28),
  gpio_lev0  = (GPIO_BASE + 0x34),
};

#define set_pin(pin) *((volatile uint32_t*)addr_set) = 1 << (pin)
#define clr_pin(pin) *((volatile uint32_t*)addr_clr) = 1 << (pin)
#define read_pin(pin) ((*((volatile uint32_t*)addr_lvl) >> (pin)) & 1)

#define wait_rtck(v) \
  while (read_pin(JTAG_RTCK) != v);

#define toggle_clk_0(s) \
  do { \
    clr_pin(JTAG_TCK); \
    wait_rtck(0); \
    /* delay_ncycles(s, cycle_half); */ \
    s += cycle_half; \
  } while (0)

#define toggle_clk_1(s) \
  do { \
    set_pin(JTAG_TCK); \
    wait_rtck(1); \
    /* delay_ncycles(s, cycle_half); */ \
    s += cycle_half; \
  } while (0)

#define toggle_clk_01(s) \
  do { \
    toggle_clk_0(s); \
    toggle_clk_1(s); \
  } while (0)

#define toggle_clk_10(s) \
  do { \
    toggle_clk_1(s); \
    toggle_clk_0(s); \
  } while (0)

typedef struct {
  uint8_t tms : 1;
  uint8_t tdi : 1;
} reading_t;

void read_jtag(
    uint32_t addr_set, uint32_t addr_clr, uint32_t addr_lvl,
    uint32_t cycle_half) { 
  gpio_set_input(JTAG_TRST);
  gpio_set_output(JTAG_RTCK);
  gpio_set_output(JTAG_TDO);
  gpio_set_input(JTAG_TCK);
  gpio_set_input(JTAG_TDI);
  gpio_set_input(JTAG_TMS);

  // DEBUG
  set_pin(JTAG_TDO);

  enum {
    N = 900,
    TIMEOUT = (int)700e6 // 1 second timeout
  };
  reading_t readings[N];

  // Wait until the first transition: 0 -> 1
  while (!read_pin(JTAG_TCK));
  set_pin(JTAG_RTCK);

  int n;
  for (n = 0; n < N;) {
    readings[n++] = (reading_t){
      .tms = read_pin(JTAG_TMS),
      .tdi = read_pin(JTAG_TDI),
    };

    uint32_t s = cycle_cnt_read();
    int in_time = 1;
    // 1 -> 0
    while (read_pin(JTAG_TCK) && (in_time = (cycle_cnt_read() - s < TIMEOUT)));
    if (!in_time) break;
    clr_pin(JTAG_RTCK);
    // 0 -> 1
    while (!read_pin(JTAG_TCK) && (in_time = (cycle_cnt_read() - s < TIMEOUT)));
    if (!in_time) break;
    set_pin(JTAG_RTCK);
  }

  for (int i = 0; i < n; ++i)
    output("time: %d, tms: %d, tdi: %d\n", i, readings[i].tms, readings[i].tdi);
}

void notmain(void) {
  caches_enable();
  read_jtag(gpio_set0, gpio_clr0, gpio_lev0, CLK_CYCLES_HALF);
}
