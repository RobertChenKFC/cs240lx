#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"

/*
 * vector base address register:
 *   3-121 --- let's us control where the exception jump table is!
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

// return the current value vector base is set to.
cp_asm_get(vector_base_fn, p15, 0, c12, c0, 0)
static inline void *vector_base_get(void) {
  return (void*)vector_base_fn_get();
}

// check that not null and alignment is good.
static inline int vector_base_chk(void *vector_base) {
  return vector_base != NULL && ((unsigned)vector_base & 0b11111) == 0;
}

// set vector base: must not have been set already.
cp_asm_set(vector_base_fn, p15, 0, c12, c0, 0)
static inline void vector_base_set(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *v = vector_base_get();
    // if already set to the same vector, just return.
    if(v == vec)
        return;

    if(v) 
        panic("vector base register already set=%p\n", v);

    vector_base_fn_set((unsigned)vec);

    // make sure it equals <vec>
    v = vector_base_get();
    if(v != vec)
        panic("set vector=%p, but have %p\n", vec, v);
}

// set vector base to <vec> and return old value: could have
// been previously set (i.e., non-null).
static inline void *
vector_base_reset(void *vec) {
    void *old_vec = 0;

    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    old_vec = vector_base_get();
    vector_base_fn_set((unsigned)vec);

    assert(vector_base_get() == vec);
    return old_vec;
}
#endif
