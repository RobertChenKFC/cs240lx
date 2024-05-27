#ifndef JTAG_H
#define JTAG_H

#include "rpi.h"
#include <stdbool.h>

enum {
  // Pin Definitions
  JTAG_RTCK = 23,
  JTAG_TDO = 24,
  JTAG_TCK = 25,
  JTAG_TDI = 26,
  JTAG_TMS = 27,

  // Instructions
  JTAG_IR_LEN = 5,
  JTAG_EXTEST = 0b00000,
  JTAG_SCAN_N = 0b00010,
  JTAG_RESTART = 0b00100,
  JTAG_HALT = 0b01000,
  JTAG_INTEST = 0b01100,
  JTAG_ITRSEL = 0b11101,
  JTAG_IDCODE = 0b11110,
  JTAG_BYPASS = 0b11111,

  // ID Code
  JTAG_ID_LEN = 32,

  // Scan Register
  JTAG_SCREG_LEN = 5,
  JTAG_SCREG_DIDR = 0,
  JTAG_SCREG_DSCR = 1,
  JTAG_SCREG_ITR = 4,
  JTAG_SCREG_DTR = 5,

  // DIDR
  JTAG_DIDR_LEN = 32,
  JTAG_IMPLEMENTOR_LEN = 8,

  // DSCR
  JTAG_DSCR_LEN = 32,
  JTAG_DSCR_RDTR_FULL = 1 << 30,
  JTAG_DSCR_HALT_DBG = 1 << 14,
  JTAG_DSCR_EXEC_ARM_EN = 1 << 13,
  JTAG_DSCR_DATA_ABORT = 1 << 7,
  JTAG_DSCR_RESTART = 1 << 1,
  JTAG_DSCR_HALT = 1 << 0,

  // ITR
  JTAG_ITR_LEN = 33,

  // DTR
  JTAG_DTR_LEN = 34,

  // Registers
  JTAG_REG_PC = 15,
  JTAG_REG_CPSR = 25,

  // CPSR
  JTAG_CPSR_JAZELLE = 1 << 24,
  JTAG_CPSR_THUMB = 1 << 5,

  // BCR
  JTAG_BCR_MEANING_BITS = 0b11 << 21,
  JTAG_BCR_IMVA_MATCH = 0b00 << 21,
  JTAG_BCR_IMVA_MISMATCH = 0b10 << 21,
  JTAG_BCR_LINKING = 0b1 << 20,
  JTAG_BCR_SECURE_BITS = 0b11 << 14,
  JTAG_BCR_SECURE_NONSECURE = 0b00 << 14,
  JTAG_BCR_BYTE_ADDR_SEL_BITS = 0b1111 << 5,
  JTAG_BCR_SUPER_BITS = 0b11 << 1,
  JTAG_BCR_PRIV_USER = 0b11 << 1,
  JTAG_BCR_EN = 0b1
};

// Low-level JTAG stuff
void jtag_init(void);
void jtag_reset(void);
uint32_t jtag_set_ir(uint32_t ir);
void jtag_set_dr_long(uint32_t *dr, uint32_t len, uint32_t *old_dr);
uint32_t jtag_set_dr(uint32_t dr, uint32_t len);

// High-level debugger operations
void jtag_set_screg(uint32_t screg);
uint32_t jtag_read_idcode(void);
void jtag_check_idcode(void);
void jtag_read_didr_long(uint32_t *didr, uint32_t *implementor);
uint32_t jtag_read_didr(void);
void jtag_check_didr(void);
uint32_t jtag_read_dscr(void);
void jtag_write_dscr(uint32_t dscr);
void jtag_prepare_dscr(void);
void jtag_halt(void);
uint32_t jtag_halted(void);
void jtag_restart(void);
uint32_t jtag_restarted(void);
void jtag_enter_debug_state(void);
void jtag_exit_debug_state(void);
void jtag_run_inst(uint32_t inst);
uint32_t jtag_try_read_dtr(uint32_t *dtr);
uint32_t jtag_read_dtr(void);
void jtag_write_dtr(uint32_t dtr);
uint32_t jtag_read_register(uint32_t r);
uint32_t jtag_read_original_register(uint32_t r);
void jtag_write_register(uint32_t r, uint32_t x);
uint8_t jtag_read_memory(uint32_t addr);
void jtag_write_memory(uint32_t addr, uint8_t x);
uint32_t jtag_read_bcr(void);
void jtag_write_bcr(uint32_t val);
uint32_t jtag_read_bvr(void);
void jtag_write_bvr(uint32_t addr);
void jtag_step(void);
void jtag_continue(void);

#endif // JTAG_H
