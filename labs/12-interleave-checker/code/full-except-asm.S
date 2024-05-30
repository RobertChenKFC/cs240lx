@ this file defines the trampolines used to forward exception/interrupts
@ to <full-except.c>
@
@ each should:
@   1. save all the registers (spsr, r0-r15) onto the stack
@      in a <regs_t> layout.  (as before: r0 at offset 0, 
@      r1 at byte offset 4, etc)
@
@   2. call the associated full except handler in <full-except.c>
@
@      the key property: if we call <switchto> on these 
@      saved registers execution should resume in "the
@      right place"  --- for exceptions/interrupts
@      exactly where the exception occured, for the
@      system call one instructon after.
@
@  currently we only define three full exception handlers, you can 
@  add the rest.

#include "rpi-asm.h"

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@  
@ these are the non "full except" handlers that work just like all
@ the exceptions/interrupts we've done so far this quarter:
@ just save registers and call the default name.  
@
@ included so you have a handy way to see how to use macros in
@ assembly.
@ 
@ you don't need to modify this (but you can to make everything
@ be a full_except style handler.

@ save all the stuff we need, then call the given routine.
#define TRAMPOLINE(fn)              \
  mov sp, #INT_STACK_ADDR;          \
  push  {r0-r12,lr};                \
  mov   r0, lr;                     \
  bl    fn;                         \
  pop   {r0-r12,lr};                \
  movs    pc, lr

// keep the name of the trampoline private.
#define MK_TRAMPOLINE(name, offset, fn)  \
    name: sub   lr, lr, # (offset); TRAMPOLINE(fn)

@ call the default handlers (if client doesn't define these will just
@ call the libpi versions that panic with an error).
@
@ you can migrate these to full_except if you want!
MK_TRAMPOLINE(reset,            4, reset_vector)
MK_TRAMPOLINE(undef,            4, undefined_instruction_vector)
MK_TRAMPOLINE(prefetch_abort,   4, prefetch_abort_vector)
MK_TRAMPOLINE(interrupt,        4, int_vector)


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ this is the code you have to write.


@ save all registers into a <regs_t> (same as last lab)
@ and call the full-except.c:prefetch_abort_full_except
@ handler.

@ what to do:
@  1. load the stack into sp
@  2. adjust lr;
@  3. save all the regs into a <regs_t>
@  3. call <full-except.c:prefetch_abort_full_except> with
@       <reg_t> in <r0>
@       <spsr> in <r1>
@       <pc> in <r2>
@
@ NOTE: 
@  - save registers assuming we come from user mode.
@    <full-except.c> will patch up if needed.
@  - the pc in <regs> should point to the instruction that
@    faulted.
prefetch_handler_full:
    @ initially we just forward to staff
    @ b staff_prefetch_trampoline_full

    @ DEBUG
    @ mov sp, #INT_STACK_ADDR
    sub   lr, lr, #4

    sub sp, sp, #68
    stm sp, {r0-r14}^
    str lr, [sp, #60]
    mrs r1, spsr
    str r1, [sp, #64]

    mov r0, sp
    mov r2, lr

    bl prefetch_abort_full_except
    asm_not_reached();

@ NOTE: 
@  - save registers assuming we come from user mode.
@    <full-except.c> will patch up if needed.
@  - the pc in <regs> should point to the instruction that
@    faulted.
data_abort_full:
    @ initially we just forward to staff
    @ b staff_data_abort_trampoline_full

    @ DEBUG
    @ mov sp, #INT_STACK_ADDR
    sub   lr, lr, #8

    sub sp, sp, #68
    stm sp, {r0-r14}^
    str lr, [sp, #60]
    mrs r1, spsr
    str r1, [sp, #64]

    mov r0, sp
    mov r2, lr
    bl data_abort_full_except
    asm_not_reached();

@ NOTE: 
@  - assumes we come from user mode.
@  - assumes we have a legit sp so don't set it.
@  - save registers assuming we come from user mode.
@
@ the pc in <regs> should point to the instruction after.
syscall_full:
    @ initially we just forward to staff
    @ b staff_syscall_trampoline_full

    @ DEBUG
    @ mov sp, #INT_STACK_ADDR

    sub sp, sp, #68
    stm sp, {r0-r14}^
    str lr, [sp, #60]
    mrs r1, spsr
    str r1, [sp, #64]

    mov r0, sp
    mov r2, lr
    bl syscall_full_except
    asm_not_reached();

undef_full:
    @ initially we just forward to staff
    @ b staff_syscall_trampoline_full

    @ DEBUG
    @ mov sp, #INT_STACK_ADDR

    sub sp, sp, #68
    stm sp, {r0-r14}^
    str lr, [sp, #60]
    mrs r1, spsr
    str r1, [sp, #64]

    mov r0, sp
    mov r2, lr
    bl undef_full_except
    asm_not_reached();

interrupt_full:
    @ DEBUG
    @ mov sp, #INT_STACK_ADDR
    sub sp, sp, #68

    @ Save r0-r2 (same in any mode)
    stm sp, {r0-r2}

    @ Increment sp by 12 (for later stm)
    add sp, sp, #12

    @ Check if spsr is user
    mrs r0, spsr
    and r0, #0b11111
    cmp r0, #USER_MODE
    beq interrupt_full_user_mode
    @ Privileged mode
    @ Save sp to r0
    mov r0, sp
    @ Save current mode to r2
    mrs r2, cpsr
    @ Change mode to spsr with interrupts disabled
    mrs r1, spsr
    orr r1, r1, #(0b11 << 7)
    msr cpsr_cxsf, r1
    prefetch_flush(r1);
    @ Save r3-r14
    stm r0, {r3-r14}
    @ Change back to original mode
    msr cpsr_cxsf, r2
    prefetch_flush(r1);
    b interrupt_full_end
interrupt_full_user_mode:
    @ User mode
    stm sp, {r3-r14}^
interrupt_full_end:
    @ Set sp to start of register array (top of stack)
    sub sp, sp, #12
    @ Set sp to end of register array
    @ Save PC before interrupt
    sub lr, lr, #4
    str lr, [sp, #60]
    @ Save SPSR
    mrs r1, spsr
    str r1, [sp, #64]

    mov r0, sp
    mov r2, lr
    bl interrupt_full_except
    asm_not_reached();

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ do not have to modify
@
@ standard vector that we can give to vector base.
@ no different than other labs, other than what
@ we call.


.align 5; 
.globl full_except_ints
full_except_ints: 
    b reset                 @ 0, generic
    b undef_full            @ 1, full
    b syscall_full          @ 2, full
    b prefetch_handler_full @ 3, full
    b data_abort_full       @ 4, full
    b reset                 @ 5, generic
    b interrupt_full        @ 6, full
    b bad_exception

.globl init_sp
init_sp:
    mrs r0, cpsr
    bic r1, r0, #0b11111

    orr r2, r1, #ABORT_MODE
    msr cpsr_cxsf, r2
    mov sp, #INT_STACK_ADDR

    orr r2, r1, #UNDEF_MODE
    msr cpsr_cxsf, r2
    mov sp, #INT_STACK_ADDR

    msr cpsr_cxsf, r0
    bx lr


bad_exception:
  asm_bad_exception("should not reach here")

