#include "rpi.h"
#include "cycle-util.h"
#include "jtag.h"

void read_idcode(void) {
  jtag_init();

  jtag_set_ir(JTAG_IDCODE);
  uint32_t id = jtag_set_dr(0, JTAG_ID_LEN);

  debug("identifier: %b\n", id);
  debug("version: %b\n", id >> 28);
  debug("part number: %b\n", (id >> 12) & 0xffff);
  debug("manufacturer id: %b\n", (id >> 1) & 0xfff);
  debug("last bit: %b\n", id & 1);
  assert(id == 0b111101101110110000101111111);
}

void read_didr(void) {
  jtag_init();

  jtag_set_ir(JTAG_SCAN_N);
  jtag_set_dr(JTAG_SCREG_DIDR, JTAG_SCREG_LEN);
  jtag_set_ir(JTAG_INTEST);
  uint32_t buf[2];
  jtag_set_dr_long(buf, JTAG_DIDR_LEN + JTAG_IMPLEMENTOR_LEN, buf);

  debug("didr: %x, implementor: %x\n", buf[0], buf[1]);
}

void read_dscr(void) {
  jtag_init();

  jtag_set_ir(JTAG_SCAN_N);
  jtag_set_dr(JTAG_SCREG_DIDR, JTAG_SCREG_LEN);
  jtag_set_ir(JTAG_INTEST);
  uint32_t dscr = jtag_set_dr(0, JTAG_DSCR_LEN);

  debug("dscr: %b\n", dscr);
}

void halt(void) {
  jtag_init();
  jtag_set_ir(JTAG_HALT);
  read_dscr();
}

void restart(void) {
  jtag_init();
  jtag_set_ir(JTAG_RESTART);
  read_dscr();
}

void notmain(void) {
  caches_enable();
  output("Starting JTAG probe...\n");

  // read_idcode();
  // read_didr();
  // read_dscr();
  // halt();
  restart();
}
