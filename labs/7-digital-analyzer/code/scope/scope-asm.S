#include "rpi-asm.h"

MK_FN(scope_asm)
  @ arguments:
  @ r0: array (of int)
  @ r1: n_max
  @ r2: max_iter
  @ returns: one past the address of last written entry (int*)

  push {r4-r12,lr}

  @ r5: max iterations
  mov r5, r2
  @ r6: array
  mov r6, r0
  @ r7: array end
  lsl r1, r1, #2
  add r7, r6, r1
  @ r0: GPIO address
  ldr r0, =0x20200034
  
  ldr r1, [r0]
  lsr r1, r1, #21

poll_loop_1:
  @ spin until GPIO doesn't match r1
  ldr r2, [r0]
  cmp r1, r2, lsr #21
  beq poll_loop_1
  @ r3: start cycle
  mrc p15, 0, r3, c15, c12, 1
  lsr r2, r2, #21

scope_loop:
poll_loop_2:
  @ spin until GPIO doesn't match r2, or max iter is decremented to 0
  subs r5, r5, #1
  beq scope_asm_end
  ldr r1, [r0]
  cmp r2, r1, lsr #21
  beq poll_loop_2
  @ calculate the number of cycles elapsed from start
  mrc p15, 0, r4, c15, c12, 1
  sub r4, r4, r3
  @ store cycle count in array
  str r4, [r6]
  @ increment array and exit if reach end
  add r6, r6, #4
  cmp r6, r7
  beq scope_asm_end
  @ move the GPIO value read to r2
  lsr r2, r1, #21

  @ DEBUG
  @ mov r0, r5
  @ bl print_value

  b scope_loop

scope_asm_end:
  @ return end of array
  mov r0, r6

  pop {r4-r12,lr}
  bx lr

@ just use the default int stack address.
#define INT_STACK INT_STACK_ADDR

@ these are all unexpected.
#define MK_TRAMPOLINE(name, offset, fn)         \
    name:                                       \
        sub   lr, lr, # (offset);               \
        mov sp, #INT_STACK;               \
        push  {r0-r12,lr};                \
        mov   r0, lr;                     \
        bl    fn;                         \
        pop   {r0-r12,lr};                \
        movs    pc, lr


@ call the default handlers and blow up.  override if you
@ want to do something fancier.
MK_TRAMPOLINE(reset,            4, reset_vector)
MK_TRAMPOLINE(undef,            4, undefined_instruction_vector)
MK_TRAMPOLINE(prefetch_abort,   4, prefetch_abort_vector)
MK_TRAMPOLINE(data_abort,       8, data_abort_vector)
MK_TRAMPOLINE(syscall,          4, syscall_vector)

@ you would read cycle counter and pass it to <int_vector>
@ in r1.
interrupt:
        mcr p15, 0, sp, c13, c0, 2     @ write the cycle count into a scratch register
        sub   lr, lr, #4               @ correct interrupt pc
        mov sp, #INT_STACK             @ load the stack pointer
        push  {r0-r12,lr}              @ push all regs (trim this to caller)
        mov   r0, lr                   @ pass exception pc as arg0
        mrc p15, 0, r1, c13, c0, 2     @ mov the cycle count into arg1
        bl    int_vector               @ call our interrupt handler.
        pop   {r0-r12,lr}              @ pop all the regs
        movs    pc, lr                 @ resume back at exception location.

.align 5; 
.globl default_vec_ints
default_vec_ints: 
    b reset
    b undef
    b syscall
    b prefetch_abort
    b data_abort
    b reset
    @ ugly hack, since this is the last interrupt handler, this works
    @ DEBUG
    mov r0, r6
    mov sp, #INT_STACK
    bl int_handler
    /*
    mrc p15, 0, sp, c15, c12, 1    @ read cycle count into sp
    mcr p15, 0, sp, c13, c0, 2     @ write the cycle count into a scratch register
    sub   lr, lr, #4               @ correct interrupt pc
    mov sp, #INT_STACK             @ load the stack pointer
    push  {r0-r12,lr}              @ push all regs (trim this to caller)
    mov   r0, lr                   @ pass exception pc as arg0
    mrc p15, 0, r1, c13, c0, 2     @ mov the cycle count into arg1
    bl    int_vector               @ call our interrupt handler.
    pop   {r0-r12,lr}              @ pop all the regs
    movs    pc, lr                 @ resume back at exception location.
    */
