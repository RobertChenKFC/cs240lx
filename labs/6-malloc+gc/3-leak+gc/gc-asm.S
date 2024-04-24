#include "rpi-asm.h"

@ store all registers into the the array passed in <r0>
@ and return.
MK_FN(dump_regs)
  stm r0, {r0-r15}
  bx lr

MK_FN(get_sp)
  mov r0, sp
  bx lr
