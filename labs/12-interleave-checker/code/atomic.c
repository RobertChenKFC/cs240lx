#include "atomic.h"
#include "syscalls.h"
#include "check-interleave.h"

int in_user_mode(void) {
  return mode_get(cpsr_get()) == USER_MODE;
}

void *malloc(unsigned nbytes) {
#ifndef NO_SYSCALL
  if (in_user_mode())
    return (void*)syscall_invoke_asm(SYS_MALLOC, nbytes);
#endif
  return kmalloc(nbytes);
}

void atomic_init(volatile atomic_intptr_t *obj, intptr_t desired) {
#ifndef NO_SYSCALL
  if (in_user_mode())
    syscall_invoke_asm(SYS_ATOMIC_INIT, obj, desired);
  else
#endif
    atomic_init_impl(obj, desired);
}

intptr_t atomic_load(const volatile atomic_intptr_t *obj) {
#ifdef NO_SYSCALL
  return atomic_load_impl(obj);
#else
  return (intptr_t)syscall_invoke_asm(SYS_ATOMIC_LOAD, obj);
#endif
}

int atomic_compare_exchange_weak(
    volatile atomic_intptr_t *obj, volatile atomic_intptr_t *expected,
    intptr_t desired) {
#ifdef NO_SYSCALL
  return atomic_compare_exchange_weak_impl(obj, expected, desired);
#else
  return syscall_invoke_asm(SYS_ATOMIC_COMP_EXCHG, obj, expected, desired);
#endif
}

int atomic_exchange(volatile atomic_int *obj, int desired) {
#ifdef NO_SYSCALL
  return atomic_exchange_impl(obj, desired);
#else
  return syscall_invoke_asm(SYS_ATOMIC_EXCHG, obj, desired);
#endif
}

void atomic_init_impl(volatile atomic_intptr_t *obj, intptr_t desired) {
  *obj = (volatile atomic_intptr_t)desired;
}

intptr_t atomic_load_impl(const volatile atomic_intptr_t *obj) {
  return (intptr_t)*obj;
}

int atomic_compare_exchange_weak_impl(
    volatile atomic_intptr_t *obj, volatile atomic_intptr_t *expected,
    intptr_t desired) {
  if (*obj == *expected) {
    *obj = (volatile atomic_intptr_t)desired;
    return 1;
  }
  *expected = *obj;
  return 0;
}

int atomic_exchange_impl(volatile atomic_int *obj, int desired) {
  int ret = *obj;
  *obj = desired;
  return ret;
}
