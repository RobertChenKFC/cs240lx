#include "rpi.h"

void hello_before(void) { 
    printk("hello world before\n");
}

// i would call this instead of printk if you have problems getting
// ldr figured out.
void foo(int x) { 
    printk("foo was passed %d\n", x);
}

uint32_t arm_bl(uint32_t src_addr, uint32_t target_addr) {
  // Last 24 bits are the branch offset

  uint32_t offset;
  // src + 8 + 4 * offset = dst
  // offset = (dst - src - 8) / 4
  if (target_addr >= src_addr) {
    offset = (target_addr - src_addr - 8) / 4;
  } else {
    offset = (src_addr - target_addr + 8) / 4;
    offset = ~offset;
    ++offset;
    offset &= 0xffffff;
  }

  uint32_t inst = 0xeb000000 | offset;
  return inst;
}

void notmain(void) {
    /*
    {
      // DEBUG
      uint32_t __attribute__((aligned(4))) code[1024];
      code[0] = 0xe92d400f; // push {r0, r1, r2, r3, lr}
      code[1] = 0xe59f0008; // ldr r0, [pc, #8]
      code[2] = arm_bl((uint32_t)(code + 2), (uint32_t)printk);
      code[3] = 0xe8bd400f; // pop {r0, r1, r2, r3, lr}
      code[4] = 0xe12fff1e; // bx lr
      code[5] = (uint32_t)"Hello world before\n";
      ((void (*)(void))code)();
    }
    */

  // DEBUG
  printk("bl = %x\n", arm_bl(36312, 34172));

    // generate a dynamic call to hello_before() 
    // 1. you'll have to save/restore registers lr
    // 2. load the string address [likely using ldr]
    // 3. call printk
    /*
00008038 <hello_before>:
    8038:	e92d4010 	push	{r4, lr}
    803c:	e59f0004 	ldr	r0, [pc, #4]	; 8048 <hello_before+0x10>
    8040:	eb000033 	bl	8114 <printk>
    8044:	e8bd8010 	pop	{r4, pc}
    8048:	00008b68 	.word	0x00008b68
    */
    static uint32_t code[16] = {
      0xe92d400f, // push {r0, r1, r2, r3, lr}
      0xe59f0008, // ldr r0, [pc, #8]
      0,          // bl <printk>
      0xe8bd400f, // pop {r0, r1, r2, r3, lr}
      0xe12fff1e, // bx lr
      0           // .word <hello_before_str>
    };
    code[2] = arm_bl((uint32_t)(code + 2), (uint32_t)printk);
    code[5] = (uint32_t)"Hello world before\n";
    unsigned n = 5;

    // DEBUG TESTS
    uint32_t srcs[] = { 0x80a8, 0x80b8, 0x80e4 };
    uint32_t dsts[] = { 0x816c, 0x804c, 0x8038 };
    uint32_t inst[] = { 0xeb00002f, 0xebffffe3, 0xebffffd3 };
    int ntests = 3;
    for (int i = 0; i < ntests; ++i)
      assert(arm_bl(srcs[i], dsts[i]) == inst[i]);

    // do this both for hello_before() and hello_after()
    printk("emitted code:\n");
    for(int i = 0; i < n; i++) 
        printk("code[%d]=0x%x\n", i, code[i]);

    void (*fp)(void) = (typeof(fp))code;
    printk("about to call: %x\n", fp);
    printk("--------------------------------------\n");
    fp();
    printk("--------------------------------------\n");
    printk("success!\n");
    clean_reboot();
}

void hello_after(void) { 
    printk("hello world after\n");
}
