#include "jtag.h"

enum {
  GPIO_BASE = 0x20200000,
  gpio_fsel0 = (GPIO_BASE + 0x00),
  gpio_set0  = (GPIO_BASE + 0x1C),
  gpio_clr0  = (GPIO_BASE + 0x28),
  gpio_lev0  = (GPIO_BASE + 0x34),
};

#define set_pin(pin) *((volatile uint32_t*)gpio_set0) = 1 << (pin)
#define clr_pin(pin) *((volatile uint32_t*)gpio_clr0) = 1 << (pin)
#define read_pin(pin) ((*((volatile uint32_t*)gpio_lev0) >> (pin)) & 1)

#define wait_rtck(v) \
  while (read_pin(JTAG_RTCK) != v);

#define toggle_clk_0 \
  do { \
    clr_pin(JTAG_TCK); \
    wait_rtck(0); \
  } while (0)

#define toggle_clk_1 \
  do { \
    set_pin(JTAG_TCK); \
    wait_rtck(1); \
  } while (0)

#define toggle_clk_01 \
  do { \
    toggle_clk_0; \
    toggle_clk_1; \
  } while (0)

#define toggle_clk_10 \
  do { \
    toggle_clk_1; \
    toggle_clk_0; \
  } while (0)

void jtag_init(void) {
  gpio_set_input(JTAG_RTCK);
  gpio_set_input(JTAG_TDO);
  gpio_set_output(JTAG_TCK);
  gpio_set_output(JTAG_TDI);
  gpio_set_output(JTAG_TMS);

  jtag_reset();
}

void jtag_reset(void) {
  // Go to Test-Logic-Reset
  set_pin(JTAG_TMS);
  for (int i = 0; i < 10; ++i)
    toggle_clk_01;

  // Go to Run-Test/Idle
  clr_pin(JTAG_TMS);
  toggle_clk_01;
}

uint32_t jtag_set_ir(uint32_t ir) {
  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Select-IR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Capture-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Shift-IR
  clr_pin(JTAG_TMS);
  toggle_clk_01;

  // Shift in the IR
  uint32_t old_ir = 0;
  for (int i = 0; i < JTAG_IR_LEN; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    uint32_t v = (ir >> i) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == JTAG_IR_LEN - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);

    old_ir |= read_pin(JTAG_TDO) << i;
  }

  // Go to Update-IR
  set_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Run-Test-Idle 
  clr_pin(JTAG_TMS);
  toggle_clk_01;

  return old_ir;
}

void jtag_set_dr_long(uint32_t *dr, uint32_t len, uint32_t *old_dr) {
  // Go to Select-DR-Scan
  set_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Capture-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Shift-DR
  clr_pin(JTAG_TMS);
  toggle_clk_01;

  // Shift in the DR
  for (uint32_t i = 0; i < len; ++i) {
    clr_pin(JTAG_TCK);
    wait_rtck(0);

    uint32_t word_idx = i / 32, bit_idx = i % 32;
    if (bit_idx == 0)
      old_dr[word_idx] = 0;
    uint32_t v = (dr[word_idx] >> bit_idx) & 1;
    if (v)
      set_pin(JTAG_TDI);
    else
      clr_pin(JTAG_TDI);
    if (i == len - 1)
      set_pin(JTAG_TMS);

    set_pin(JTAG_TCK);
    wait_rtck(1);

    old_dr[word_idx] |= read_pin(JTAG_TDO) << bit_idx;
  }

  // Go to Update-DR
  set_pin(JTAG_TMS);
  toggle_clk_01;

  // Go to Run-Test-Idle
  clr_pin(JTAG_TMS);
  toggle_clk_01;
}

uint32_t jtag_set_dr(uint32_t dr, uint32_t len) {
  uint32_t old_dr;
  jtag_set_dr_long(&dr, len, &old_dr);
  return old_dr;
}
