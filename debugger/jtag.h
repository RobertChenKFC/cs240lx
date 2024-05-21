#ifndef JTAG_H
#define JTAG_H

enum {
  JTAG_TRST = 22,
  JTAG_RTCK = 23,
  JTAG_TDO = 24,
  JTAG_TCK = 25,
  JTAG_TDI = 26,
  JTAG_TMS = 27,

  JTAG_IR_LEN = 5,

  JTAG_EXTEST = 0b00000,
  JTAG_SCAN_N = 0b00010,
  JTAG_RESTART = 0b00100,
  JTAG_HALT = 0b01000,
  JTAG_INTEST = 0b01100,
  JTAG_ITRSEL = 0b11101,
  JTAG_IDCODE = 0b11110,
  JTAG_BYPASS = 0b11111,

  JTAG_SCREG_LEN = 5,
  JTAG_SCREG_DIDR = 0,
  JTAG_SCREG_DSCR = 1,
  JTAG_SCREG_ITR = 4,

  JTAG_DIDR_LEN = 32,
  JTAG_IMPLEMENTOR_LEN = 8,
};

#endif // JTAG_H
