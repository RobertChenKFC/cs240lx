#ifndef ATOMIC_H
#define ATOMIC_H

#include "rpi.h"

typedef uint32_t *atomic_intptr_t;
typedef int atomic_int;

void *malloc(unsigned nbytes);
void atomic_init(volatile atomic_intptr_t *obj, intptr_t desired);
intptr_t atomic_load(const volatile atomic_intptr_t *obj);
int atomic_compare_exchange_weak(
    volatile atomic_intptr_t *obj, volatile atomic_intptr_t *expected,
    intptr_t desired);
int atomic_exchange(volatile atomic_int *obj, int desired);

void atomic_init_impl(volatile atomic_intptr_t *obj, intptr_t desired);
intptr_t atomic_load_impl(const volatile atomic_intptr_t *obj);
int atomic_compare_exchange_weak_impl(
    volatile atomic_intptr_t *obj, volatile atomic_intptr_t *expected,
    intptr_t desired);
int atomic_exchange_impl(volatile atomic_int *obj, int desired);

#endif // ATOMIC_H
