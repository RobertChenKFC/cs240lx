#include "rpi-asm.h"

MK_FN(net_send1)
  push {r4}

  ldr r4, =0x20200000


  pop {r4}
  bx lr
