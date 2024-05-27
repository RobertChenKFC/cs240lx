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

void jtag_set_screg(uint32_t screg) {
  jtag_set_ir(JTAG_SCAN_N);
  jtag_set_dr(screg, JTAG_SCREG_LEN);
}

uint32_t jtag_read_idcode(void) {
  jtag_set_ir(JTAG_IDCODE);
  return jtag_set_dr(0, JTAG_ID_LEN);
}

void jtag_check_idcode(void) {
  uint32_t id = jtag_read_idcode();
  debug("identifier: %b\n", id);
  debug("version: %b\n", id >> 28);
  debug("part number: %b\n", (id >> 12) & 0xffff);
  debug("manufacturer id: %b\n", (id >> 1) & 0xfff);
  debug("last bit: %b\n", id & 1);
  assert(id == 0b111101101110110000101111111);
}

void jtag_read_didr_long(uint32_t *didr, uint32_t *implementor) {
  jtag_set_screg(JTAG_SCREG_DIDR);
  jtag_set_ir(JTAG_INTEST);
  uint32_t buf[2];
  jtag_set_dr_long(buf, JTAG_DIDR_LEN + JTAG_IMPLEMENTOR_LEN, buf);
  *didr = buf[0];
  *implementor = buf[1];
}

uint32_t jtag_read_didr(void) {
  uint32_t didr, implementor;
  jtag_read_didr_long(&didr, &implementor);
  return didr;
}

void jtag_check_didr(void) {
  uint32_t didr, implementor;
  jtag_read_didr_long(&didr, &implementor);
  debug("didr: %x\n", didr);
  debug("implementor: %x\n", implementor);
  assert((didr >> 12) == 0x15121);
  assert(implementor == 0x41);
}

uint32_t jtag_read_dscr(void) {
  jtag_set_screg(JTAG_SCREG_DSCR);
  jtag_set_ir(JTAG_INTEST);
  return jtag_set_dr(0, JTAG_DSCR_LEN);
}

void jtag_write_dscr(uint32_t dscr) {
  jtag_set_screg(JTAG_SCREG_DSCR);
  jtag_set_ir(JTAG_EXTEST);
  jtag_set_dr(dscr, JTAG_DSCR_LEN);
}

void jtag_halt(void) {
  jtag_set_ir(JTAG_HALT);
  while (!jtag_halted());
}

uint32_t jtag_halted(void) {
  return jtag_read_dscr() & JTAG_DSCR_HALT;
}

void jtag_restart(void) {
  jtag_set_ir(JTAG_RESTART);
  while (!jtag_restarted());
}

uint32_t jtag_restarted(void) {
  return jtag_read_dscr() & JTAG_DSCR_RESTART;
}

static uint32_t saved_dscr;
static uint32_t saved_wdtr, saved_wdtr_valid;
static uint32_t saved_r0;
static uint32_t saved_r1;
static uint32_t saved_rdtr;
static uint32_t saved_cpsr;
static uint32_t saved_pc;
void jtag_enter_debug_state(void) {
  // arm1176.pdf, p.14-31, Section 14.8.4
  // 1. check whether core has entered debug state
  jtag_halt();
  // 2. save DSCR
  saved_dscr = jtag_read_dscr();
  // 3. save wDTR
  saved_wdtr_valid = jtag_try_read_dtr(&saved_wdtr);
  // 4. set ARM instruction enable bit
  jtag_write_dscr(saved_dscr | JTAG_DSCR_EXEC_ARM_EN);
  // 6. store out r0; the order of steps 5 and 6 are swapped, because step 6
  //    requires a scratch register to run DSB, so we save r0 for use as scratch
  //    register later
  saved_r0 = jtag_read_register(0);
  // 5. Drain the write buffer
  // dsb
  jtag_run_inst(0xee070f9a);
  // According to the manual, we have to wait for an imprecise data abort after
  // running dsb, but it's getting stuck here, so I skipped this wait
  // while (!(jtag_read_dscr() & JTAG_DSCR_DATA_ABORT));
  // nop
  jtag_run_inst(0xe1a00000);
  // 7. save rDTR
  if (saved_dscr & JTAG_DSCR_RDTR_FULL) {
    // Read rDTR (cp14 debug register c5) into r0
    jtag_run_inst(0xee100e15);
    saved_rdtr = jtag_read_register(0);
  }
  // 8. save cpsr
  saved_cpsr = jtag_read_register(JTAG_REG_CPSR);
  // 9. save pc
  saved_pc = jtag_read_register(JTAG_REG_PC);
  // 10. adjust pc to resume execution later
  if (!(saved_cpsr & JTAG_CPSR_JAZELLE)) {
    if (saved_cpsr & JTAG_CPSR_THUMB)
      saved_pc -= 0x4;
    else
      saved_pc -= 0x8;
  }
  // 11. preserve MMU registers
  // TODO: fill this in later

  // We save r1 for another scratch register
  saved_r1 = jtag_read_register(1);
}

void jtag_exit_debug_state(void) {
  // arm1176.pdf, p.14-32, Section 14.8.5
  // 1. restore all standard ARM registers
  jtag_write_register(1, saved_r1);
  // 2. restore MMU registers
  // TODO: fill this in later
  // 3. ensure that rDTR and wDTR are empty
  jtag_write_register(0, 0);
  // Write r0 to rDTR
  jtag_run_inst(0xee000e15);
  assert(jtag_read_dtr() == 0);
  // 4. restore wDTR
  if (saved_wdtr_valid) {
    jtag_write_register(0, saved_wdtr);
    // Write r0 to wDTR
    jtag_run_inst(0xee000e15);
  }
  // 5. restore cpsr
  jtag_write_register(JTAG_REG_CPSR, saved_cpsr);
  // 6. restore pc
  jtag_write_register(JTAG_REG_PC, saved_pc);
  // 7. restore r0
  jtag_write_register(0, saved_r0);
  // 8. restore dscr with ARM instruction enable cleared and halting debug mode
  //    enabled
  jtag_write_dscr((saved_dscr & ~JTAG_DSCR_EXEC_ARM_EN) | JTAG_DSCR_HALT_DBG);
  // 9. restore rDTR
  if (saved_dscr & JTAG_DSCR_RDTR_FULL)
    jtag_write_dtr(saved_rdtr);
  // 10. restart
  // 11. wait until core restarted
  jtag_restart();
}

void jtag_run_inst(uint32_t inst) {
  assert(jtag_halted());

  jtag_set_screg(JTAG_SCREG_ITR);
  jtag_set_ir(JTAG_EXTEST);
  uint32_t empty_buf[2] = {0}, inst_buf[2] = {0, 1};
  inst_buf[0] = inst;
  jtag_set_dr_long(inst_buf, JTAG_ITR_LEN, empty_buf);

  inst_buf[0] = 0xe1a00000;
  while (1) {
    jtag_set_dr_long(inst_buf, JTAG_ITR_LEN, empty_buf);
    if (empty_buf[1] & 1)
      break;
  }
}

uint32_t jtag_try_read_dtr(uint32_t *dtr) {
  jtag_set_screg(JTAG_SCREG_DTR);
  jtag_set_ir(JTAG_INTEST);
  uint32_t empty_buf[2] = {0}, data_buf[2];
  jtag_set_dr_long(empty_buf, JTAG_DTR_LEN, data_buf);
  *dtr = data_buf[0];
  return data_buf[1] & 1;
}

uint32_t jtag_read_dtr(void) {
  uint32_t dtr;
  while (!jtag_try_read_dtr(&dtr));
  return dtr;
}

void jtag_write_dtr(uint32_t dtr) {
  jtag_set_screg(JTAG_SCREG_DTR);
  jtag_set_ir(JTAG_EXTEST);
  uint32_t empty_buf[2], dtr_buf[2] = {0, 0b11};
  dtr_buf[0] = dtr;
  jtag_set_dr_long(dtr_buf, JTAG_DTR_LEN, empty_buf);
}

uint32_t jtag_read_register(uint32_t r) {
  if (r == JTAG_REG_CPSR) {
    // CPSR
    // mrs r0, cpsr
    jtag_run_inst(0xe10f0000);
    return jtag_read_register(0);
  } else if (r == JTAG_REG_PC) {
    // PC
    // mov r0, pc
    jtag_run_inst(0xe1a0000f);
    return jtag_read_register(0);
  } else {
    // General purpose registers
    assert(0 <= r && r <= 14);

    // mcr p14,0,r,c0,c5,0: read register r into DTR
    //
    // these four bits are used to encode the register
    //       |
    //       v
    // 0xee000e15
    uint32_t inst = 0xee000e15 | (r << 12);
    jtag_run_inst(inst);
    uint32_t dtr = jtag_read_dtr();
    return dtr;
  }
}

uint32_t jtag_read_original_register(uint32_t r) {
  switch (r) {
    case 0:
      return saved_r0;
    case 1:
      return saved_r1;
    case JTAG_REG_PC:
      return saved_pc;
    case JTAG_REG_CPSR:
      return saved_cpsr;
    default:
      return jtag_read_register(r);
  }
}

void jtag_write_register(uint32_t r, uint32_t x) {
  if (r == JTAG_REG_CPSR) {
    // CPSR
    jtag_write_register(0, x);
    // msr r0, cpsr
    jtag_run_inst(0xe129f000);
  } else if (r == JTAG_REG_PC) {
    // PC
    jtag_write_register(0, x);
    // mov pc, r0
    jtag_run_inst(0xe1a0f000);
  } else {
    // General purpose registers
    assert(0 <= r && r <= 14);

    jtag_write_dtr(x);
    // mrc p14,0,r,c0,c5,0: write register r into DTR
    //
    // these four bits are used to encode the register
    //       |
    //       v
    // 0xee100e15
    uint32_t inst = 0xee100e15 | (r << 12);
    jtag_run_inst(inst);
  }
}

uint8_t jtag_read_memory(uint32_t addr) {
  jtag_write_register(0, addr);
  // ldrb r0, [r0]
  jtag_run_inst(0xe5d00000);
  return jtag_read_register(0) & 0xff;
}

void jtag_write_memory(uint32_t addr, uint8_t x) {
  jtag_write_register(0, addr);
  jtag_write_register(1, x);
  // strb r1, [r0]
  jtag_run_inst(0xe5c01000);
}

uint32_t jtag_read_bcr(void) {
  // mcr p14,0,r0,c0,c0,5
  jtag_run_inst(0xee100eb0);
  return jtag_read_register(0);
}

void jtag_write_bcr(uint32_t val) {
  jtag_write_register(0, val);
  // mcr p14,0,r0,c0,c0,5
  jtag_run_inst(0xee000eb0);
}

uint32_t jtag_read_bvr(void) {
  // mrc p14,0,r0,c0,c0,5
  jtag_run_inst(0xee100e90);
  return jtag_read_register(0);
}

void jtag_write_bvr(uint32_t addr) {
  jtag_write_register(0, addr);
  // mcr p14,0,r0,c0,c0,5
  jtag_run_inst(0xee000e90);
}

void jtag_step(void) {
  uint32_t bcr = jtag_read_bcr();
  bcr &= ~JTAG_BCR_MEANING_BITS;
  bcr |= JTAG_BCR_IMVA_MISMATCH;
  bcr &= ~JTAG_BCR_LINKING;
  bcr &= ~JTAG_BCR_SECURE_BITS;
  bcr |= JTAG_BCR_SECURE_NONSECURE;
  bcr |= JTAG_BCR_BYTE_ADDR_SEL_BITS;
  bcr &= ~JTAG_BCR_SUPER_BITS;
  bcr |= JTAG_BCR_PRIV_USER;
  bcr |= JTAG_BCR_EN;
  jtag_write_bcr(bcr);
  jtag_write_bvr(saved_pc);

  jtag_exit_debug_state();
  while (!jtag_halted());
  jtag_enter_debug_state();

  bcr &= ~JTAG_BCR_EN;
  jtag_write_bcr(bcr);
}
