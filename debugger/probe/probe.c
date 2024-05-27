#include "rpi.h"
#include "cycle-util.h"
#include "jtag.h"
#include "boot.h"

void boot_write8(uint8_t x) {
  uart_put8(x);
}

void boot_write32(uint32_t x) {
  boot_write8((x >> 24) & 0xff);
  boot_write8((x >> 16) & 0xff);
  boot_write8((x >>  8) & 0xff);
  boot_write8( x        & 0xff);
}

uint8_t boot_read8(void) {
  return uart_get8();
}

uint32_t boot_read32(void) {
  return (uint32_t)boot_read8() << 24 |
         (uint32_t)boot_read8() << 16 |
         (uint32_t)boot_read8() <<  8 |
         (uint32_t)boot_read8();
}

void notmain(void) {
  caches_enable();

  jtag_init();
  jtag_enter_debug_state();

#ifdef TEST
  debug("pc before step: %x\n", jtag_read_original_register(JTAG_REG_PC));
 
  jtag_step();
  
  /*
  for (int i = 0; i < 16; ++i)
    debug("r%d = %x\n", i, jtag_read_original_register(i));

  uint32_t addr = 0x8090;
  for (int i = 0; i < 4; ++i)
    debug("mem[%x] = %x\n", addr + i, (uint32_t)jtag_read_memory(addr + i));

  // DEBUG
  // bkpt #0
  uint32_t inst = 0xe1200070;
  // b 0x8000 
  // uint32_t inst = 0xeaffffda;
  for (int i = 0; i < 4; ++i)
    jtag_write_memory(addr + i, (inst >> (8 * i)) & 0xff);

  // DEBUG
  debug("wrote to memory\n");
  for (int i = 0; i < 4; ++i)
    debug("mem[%x] = %x\n", addr + i, (uint32_t)jtag_read_memory(addr + i));

  jtag_exit_debug_state();

  // DEBUG
  debug("exited debug state\n");
  debug("dscr now: %b\n", jtag_read_dscr());

  while (!jtag_halted());
  jtag_enter_debug_state();

  inst = 0xe3a0000c;
  for (int i = 0; i < 4; ++i)
    jtag_write_memory(addr + i, (inst >> (8 * i)) & 0xff);
  */

  debug("pc after step: %x\n", jtag_read_original_register(JTAG_REG_PC));
#else
  int run = 1;
  while (run) {
    uint8_t r;
    uint32_t addr;
    switch (boot_read8()) {
      case CMD_READ_REG:
        r = boot_read8();
        boot_write32(jtag_read_original_register(r));
        break;
      case CMD_READ_MEM:
        addr = boot_read32();
        boot_write8(jtag_read_memory(addr));
        break;
      case CMD_STEP:
        jtag_step();
        boot_write8(0);
        break;
      case CMD_DETACH:
        run = 0;
        break;
    }
  }
#endif

  jtag_exit_debug_state();
}
