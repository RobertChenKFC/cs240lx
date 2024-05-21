#include "rpi-asm.h"

/*
MK_FN(f)
  mov r0, #10
loop:
  subs r0, r0, #1
  bgt loop
  bx lr
*/
