#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "syscall-nums.h"

int syscall_invoke_asm(int sysno, ...);

#if 0
// resume back to user level with the current set of registers
#define sys_resume(ret)         syscall_invoke_asm(SYS_RESUME, ret)
#endif

#define sys_lock_try(addr)     syscall_invoke_asm(SYS_TRYLOCK, addr)

// a do-nothing syscall to test things.
#define sys_test(x)          syscall_invoke_asm(SYS_TEST, x)
#define sys_switchto(x)      syscall_invoke_asm(SYS_RESUME,x)

#endif
