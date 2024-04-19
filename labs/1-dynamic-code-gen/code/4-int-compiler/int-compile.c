#include "rpi.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

// you will generate this dynamically.
/*
void specialized_call_int(void) {
    int_0();
    int_1();
    int_2();
    int_3();
    int_4();
    int_5();
    int_6();
    int_7();
}
*/

enum {
  OFFSET_MASK = 0xffffff
};
uint32_t arm_b(uint32_t src_addr, uint32_t target_addr) {
  // Last 24 bits are the branch offset
  uint32_t offset;
  // src + 8 + 4 * offset = dst
  // offset = (dst - src - 8) / 4
  if (target_addr >= src_addr) {
    offset = (target_addr - src_addr - 8) / 4;
    assert(offset <= OFFSET_MASK);
  } else {
    offset = (src_addr - target_addr + 8) / 4;
    assert(offset <= OFFSET_MASK);
    offset = ~offset;
    ++offset;
    offset &= OFFSET_MASK;
  }

  uint32_t inst = 0xea000000 | offset;
  return inst;
}

uint32_t arm_bl(uint32_t src_addr, uint32_t target_addr) {
  // Last 24 bits are the branch offset
  uint32_t offset;
  // src + 8 + 4 * offset = dst
  // offset = (dst - src - 8) / 4
  if (target_addr >= src_addr) {
    offset = (target_addr - src_addr - 8) / 4;
    assert(offset <= OFFSET_MASK);
  } else {
    offset = (src_addr - target_addr + 8) / 4;
    assert(offset <= OFFSET_MASK);
    offset = ~offset;
    ++offset;
    offset &= OFFSET_MASK;
  }

  uint32_t inst = 0xeb000000 | offset;
  return inst;
}

void notmain(void) {
    int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
    };

    cycle_cnt_init();

    unsigned n = NELEM(intv);

    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    // enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    static uint32_t __attribute__((aligned(4))) code[1024];
    int idx = 0;
    code[idx++] = 0xe52de004; // push {lr}
    for (int i = 0; i < n; ++i) {
      if (i == n - 1) {
        code[idx++] = 0xe49de004; // pop {lr}
        code[idx] = arm_b((uint32_t)&code[idx], (uint32_t)intv[i]); // b intv[i]
      } else {
        code[idx] = arm_bl((uint32_t)&code[idx], (uint32_t)intv[i]); // bl intv[i]
        ++idx;
      }
    }

    void (*specialized_call_int)(void) = (void (*)(void))code;

    // rewrite to generate specialized caller dynamically.
    cnt = 0;
    TIME_CYC_PRINT10("cost of specialized int calling", specialized_call_int() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);
 
    // jump threading
    for (int i = 0; i < n - 1; ++i) {
      uint32_t *insts = (uint32_t*)intv[i];
      int j;
      // loop until we get bx lr
      for (j = 0; insts[j] != 0xe12fff1e; ++j);
      // change bx lr to b intv[i + 1]
      insts[j] = arm_b((uint32_t)&insts[j], (uint32_t)intv[i + 1]);
   }
   cnt = 0;
   TIME_CYC_PRINT10("cost of jump threading", intv[0]() );
   demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

   clean_reboot();
}
