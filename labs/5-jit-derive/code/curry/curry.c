#include "rpi.h"
#include "armv6-encodings.h"
#include "generated-encodings.h"

/*
1. allocate memory for code and to store the argument.
2. generate code to load the argument and call the original
   routine.
*/


void foo(char c, short h, int i) {
  printk("foo: %c, %d, %d\n", c, (int)h, i);
}

void print_msg(const char *msg) {
  printk("print_msg: %s\n", msg);
}

typedef void (*curry_fn)(void);

void *alloc(unsigned size) {
  static char buf[1024], *ptr = NULL;
  if (!ptr)
    ptr = buf;

  void *ret = ptr;
  if ((ptr += size) >= buf + sizeof(buf))
    panic("Ran out of space!\n");
  if (size % 4 != 0)
    ptr += 4 - size % 4;
  return ret;
}

curry_fn curry(curry_fn fp, const char *type, ...) {
  // TODO: change to dynamic allocation
  enum { CODE_SIZE = 128 };
  uint32_t *code = alloc(CODE_SIZE), *code_start = code;

  va_list args;
  va_start(args, type);

  // Caller saved registers
  *code = 0xe92d400f; // push {r0, r1, r2, r3, lr}
  ++code;

  // Move the arguments passed to this function to position after the bound
  // arguments
  int nargs = strlen(type);
  for (int i = 0; i + nargs < 4; ++i) {
    *code = armv6_mov(reg_mk(nargs), reg_mk(0));
    ++code;
  }

  // Bind the arguments at the front
  for (int i = 0; i < nargs; ++i) {
    if (i > 3)
      panic("Currently doesn't handle more than 4 arguments\n");
    switch (type[i]) {
      case 'p':
        code = armv6_load_imm32(
            code, reg_mk(i), (uint32_t)va_arg(args, void *));
        break;
      case 'i':
        code = armv6_load_imm32(code, reg_mk(i), (uint32_t)va_arg(args, int));
        break;
      case 'h':
        // TODO: change this to 16 bits
        code = armv6_load_imm32(
            code, reg_mk(i), (uint32_t)va_arg(args, int));
        break;
      case 'c':
        *code = armv6_mov_imm8(reg_mk(i), (uint8_t)va_arg(args, int));
        ++code;
        break;
      default:
        panic("bad type: <%s>\n", type);
    }
  }
  
  // Call the actual function
  *code = arm_bl((uint32_t)code, (uint32_t)fp);
  ++code;

  // Restore the caller saved registers and return
  *code = 0xe8bd400f; // pop {r0, r1, r2, r3, lr}
  ++code;
  *code = arm_bx(armv6_lr);

  assert((code - code_start) * 4 <= CODE_SIZE);
  return (curry_fn)code_start;
}



void notmain(void) {
  curry_fn foo_with_args = curry((curry_fn)foo, "chi", '@', 12345, 65537);
  foo_with_args();

  curry_fn hello = curry((curry_fn)print_msg, "p", "Hello world");
  hello();

  curry_fn foo1 = curry((curry_fn)foo, "c", 'A');
  curry_fn foo2 = curry(foo1, "h", -1234);
  curry_fn foo3 = curry(foo2, "i", -12345678);
  foo3();
}
