#include "rpi-asm.h"
#include "test-gen.h"

MK_FN(test_delay_loop)
  push {r4}
  mov r3, #0
  mov r4, #8

test_for_loop:
  ldr r2, =2997
  mrc p15, 0, r1, c15, c12, 1
test_loop:
  subs r2, r2, #1
  bgt test_loop
  mrc p15, 0, r0, c15, c12, 1
  sub r0, r0, r1
  add r3, r3, r0
  subs r4, r4, #1
  bgt test_for_loop

  pop {r4}
  lsr r3, r0, #3
  bx lr
  

#define CYCLE_DELAY 2990

MK_FN(test_gen_asm)
  push {r4-r12,lr}

  @ r0: 0th period
  ldr r0, =CYCLE_DELAY
  @ r2: gpio set address
  ldr r2, =0x2020001C
  @ r3: gpio clear address
  ldr r3, =0x20200028
  @ r4: pin
  mov r4, #(1 << 21)
  @ r6: counter
  mov r6, #5
  @ r8: even periods
  ldr r8, =CYCLE_DELAY
  @ r9: odd periods
  ldr r9, =CYCLE_DELAY

for_loop:
  @ DEBUG
  /*
  push {r0-r3,lr}
  mov r0, r9
  bl debug_function
  pop {r0-r3,lr}
  */

  @ set gpio on
  str r4, [r2]
  @ loop for 6000 cycles
  mov r7, r0
delay_loop_1:
  subs r7, r7, #1
  bgt delay_loop_1

  @ set gpio off
  str r4, [r3]
  @ loop for 6000 cycles
  mov r7, r9
delay_loop_2:
  subs r7, r7, #1
  bgt delay_loop_2

  @ decrement outer for loop
  subs r6, r6, #1
  bgt for_loop

  @ set gpio on
  str r4, [r2]
  @ loop for 6000 cycles
  mov r7, r8
delay_loop_3:
  subs r7, r7, #1
  bgt delay_loop_3

  pop {r4-r12,lr}
  bx lr
