#include "rpi-asm.h"
#include "syscall-nums.h"

@ user-level trampoline to call system call.
@
@ save lr b/c hardware will trash it (since we are at SUPER initially)
MK_FN(syscall_invoke_asm)
    push {lr}   @ we won't need these at user level.
    swi 0
    pop {lr}    @ we won't need these at user level.
    bx lr

MK_FN(sys_cswitch)
  stm r0, {r0-r14}
  str lr, [r0, #60]
  mrs r2, cpsr
  and r2, #0b11111
  str r2, [r0, #64]

  mov r0, #SYS_RESUME
  b syscall_invoke_asm
