#include "rpi-asm.h"

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
