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

void test_rtck(
    uint32_t addr_set, uint32_t addr_clr, uint32_t addr_lvl,
    uint32_t cycle_half) {
  gpio_set_output(JTAG_TRST);
  gpio_set_input(JTAG_RTCK);
  gpio_set_input(JTAG_TDO);
  gpio_set_output(JTAG_TCK);
  gpio_set_output(JTAG_TDI);
  gpio_set_output(JTAG_TMS);

  set_pin(JTAG_TRST);
  delay_ncycles(cycle_cnt_read(), 1000);

  uint32_t s = cycle_cnt_read();
  set_pin(JTAG_TRST);
  set_pin(JTAG_TMS);
  for (int i = 0; i < 10; ++i)
    toggle_clk_01(s);

  clr_pin(JTAG_TCK);
  int u = 1;
  delay_ncycles(cycle_cnt_read(), 1000);
  for (int i = 0; i < 5; ++i) {
    if (u)
      set_pin(JTAG_TCK);
    else
      clr_pin(JTAG_TCK);

    uint32_t s = cycle_cnt_read();
    while (u != read_pin(JTAG_RTCK));
    uint32_t t = cycle_cnt_read();
    debug("value: %b, cycles: %d\n", u, t - s);
    u = !u;
  }
}

void read_idcode(
    uint32_t addr_set, uint32_t addr_clr, uint32_t addr_lvl,
    uint32_t cycle_half) {
  gpio_set_output(JTAG_TRST);
  gpio_set_input(JTAG_RTCK);
  gpio_set_input(JTAG_TDO);
  gpio_set_output(JTAG_TCK);
  gpio_set_output(JTAG_TDI);
  gpio_set_output(JTAG_TMS);

  // Reset JTAG
  uint32_t s = cycle_cnt_read();
  set_pin(JTAG_TRST);
  set_pin(JTAG_TMS);
  for (int i = 0; i < 10; ++i)
    toggle_clk_01(s);

  // Go to Run-Test/Idle
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

#if 1
  // Go to Select-IR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Capture-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Write the opcode for IDcode (0b11110)
  // uint32_t opcode = JTAG_EXTEST;
  // uint32_t opcode = JTAG_SCAN_N;
  // uint32_t opcode = JTAG_RESTART;
  // uint32_t opcode = JTAG_HALT;
  // uint32_t opcode = JTAG_INTEST;
  // uint32_t opcode = JTAG_ITRSEL;
  uint32_t opcode = JTAG_IDCODE;
  // uint32_t opcode = JTAG_BYPASS;
  for (int i = 0; i < JTAG_IR_LEN; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    uint32_t v = (opcode >> i) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == JTAG_IR_LEN - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);
  }

  // Go to Update-IR
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);
#endif

  // Go to Capture-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Read the 32 bit identifier
  uint32_t id = 0;
  clr_pin(JTAG_TCK);
  s = cycle_cnt_read();
  for (int i = 0; i < 32; ++i) {
    wait_rtck(0);
    id |= read_pin(JTAG_TDO) << i;
    set_pin(JTAG_TCK);
    wait_rtck(1);
    clr_pin(JTAG_TCK);
  }

  debug("identifier: %b\n", id);
  debug("version: %b\n", id >> 28);
  debug("part number: %b\n", (id >> 12) & 0xffff);
  debug("manufacturer id: %b\n", (id >> 1) & 0xfff);
  debug("last bit: %b\n", id & 1);
  assert(id == 0b111101101110110000101111111);
}

void read_didr(
    uint32_t addr_set, uint32_t addr_clr, uint32_t addr_lvl,
    uint32_t cycle_half) {
  gpio_set_output(JTAG_TRST);
  gpio_set_input(JTAG_RTCK);
  gpio_set_input(JTAG_TDO);
  gpio_set_output(JTAG_TCK);
  gpio_set_output(JTAG_TDI);
  gpio_set_output(JTAG_TMS);

  // Reset JTAG
  uint32_t s = cycle_cnt_read();
  set_pin(JTAG_TRST);
  set_pin(JTAG_TMS);
  for (int i = 0; i < 10; ++i)
    toggle_clk_01(s);

  // Go to Run-Test/Idle
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-IR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Capture-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Write the opcode for Scan_N
  for (int i = 0; i < JTAG_IR_LEN; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    // DEBUG
    uint32_t v = (JTAG_SCAN_N >> i) & 1;
    // uint32_t v = (JTAG_IDCODE >> i) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == JTAG_IR_LEN - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);
  }

  // Go to Update-IR
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Capture-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Write the scan chain ID of DIDR to DR
  clr_pin(JTAG_TCK);
  s = cycle_cnt_read();

  // DEBUG
  uint32_t original_dr = 0;

  for (int i = 0; i < JTAG_SCREG_LEN; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    uint32_t v = (JTAG_SCREG_DIDR >> i) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == JTAG_SCREG_LEN - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);

    // DEBUG
    original_dr |= read_pin(JTAG_TDO) << i;
  }

  // DEBUG
  debug("original DR: %b\n", original_dr);

  // Go to Update-DR
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-IR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Capture-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Write the opcode for INTEST
  for (int i = 0; i < JTAG_IR_LEN; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    uint32_t v = (JTAG_INTEST >> i) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == JTAG_IR_LEN - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);
  }

  // Go to Update-IR
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Capture-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Go to Shift-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01(s);

  // Read DIDR
  uint32_t didr = 0;
  clr_pin(JTAG_TCK);
  for (int i = 0; i < JTAG_DIDR_LEN; ++i) {
    wait_rtck(0);
    didr |= read_pin(JTAG_TDO) << i;
    set_pin(JTAG_TCK);
    wait_rtck(1);
    clr_pin(JTAG_TCK);
  }
  uint32_t implementor = 0;
  clr_pin(JTAG_TCK);
  for (int i = 0; i < JTAG_IMPLEMENTOR_LEN; ++i) {
    wait_rtck(0);
    implementor |= read_pin(JTAG_TDO) << i;
    set_pin(JTAG_TCK);
    wait_rtck(1);
    clr_pin(JTAG_TCK);
  }

  debug("didr: %x, implementor: %x\n", didr, implementor);
}

void notmain(void) {
  caches_enable();

  // DEBUG
  output("Starting JTAG probe...\n");

  // DEBUG
  // test_rtck(gpio_set0, gpio_clr0, gpio_lev0, CLK_CYCLES_HALF);
  // read_idcode(gpio_set0, gpio_clr0, gpio_lev0, CLK_CYCLES_HALF);
  read_didr(gpio_set0, gpio_clr0, gpio_lev0, CLK_CYCLES_HALF);
}
