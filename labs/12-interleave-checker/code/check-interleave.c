// engler: starter code for simple A() B() interleaving checker.
// shows 
//   1. how to call the routines.
//   2. how to run code in single step mode.
//
// you'll have to rewrite some of the code to do more interesting
// checking.
#include "check-interleave.h"
#include "full-except.h"
#include "pi-sys-lock.h"
#include "atomic.h"

// used to communicate with the breakpoint handler.
static volatile checker_t *checker = 0;

int brk_verbose_p = 0;


// invoked from user level: 
//   syscall(sysnum, arg0, arg1, ...)
//
//   - sysnum number is in r->reg[0];
//   - arg0 of the call is in r->reg[1];
//   - arg1 of the call is in r->reg[2];
//   - arg2 of the call is in r->reg[3];
// we don't handle more arguments.
int syscall_handler_full(regs_t *r) {
    assert(mode_get(cpsr_get()) == SUPER_MODE);

    // we store the first syscall argument in r1
    uint32_t arg0 = r->regs[1];
    uint32_t arg1 = r->regs[2];
    uint32_t arg2 = r->regs[3];
    uint32_t sys_num = r->regs[0];

    // pc = address of instruction right after 
    // the system call.
    uint32_t pc = r->regs[15];      

    switch(sys_num) {
    case SYS_RESUME:
            switchto((void*)arg0);
            panic("not reached\n");
    case SYS_TRYLOCK: {
        pi_lock_t *l = (pi_lock_t*)arg0;
        if (*l)
          return 0;
        return *l = 1;
    }
    case SYS_TEST:
        printk("running empty syscall with arg=%d\n", arg0);
        return SYS_TEST;
    case SYS_ATOMIC_INIT:
      atomic_init_impl((volatile atomic_intptr_t *)arg0, (intptr_t)arg1);
      return 0;
    case SYS_ATOMIC_LOAD:
      return (int)atomic_load_impl((volatile atomic_intptr_t *)arg0);
    case SYS_ATOMIC_COMP_EXCHG:
      return atomic_compare_exchange_weak_impl(
          (volatile atomic_intptr_t *)arg0, (volatile atomic_intptr_t *)arg1,
          (intptr_t)arg2);
    case SYS_MALLOC:
      return (int)kmalloc((unsigned)arg0);
    case SYS_ATOMIC_EXCHG:
      return atomic_exchange_impl((volatile atomic_int *)arg0, (int)arg1);
    default:
        panic("illegal system call = %d, pc=%x, arg0=%d!\n", sys_num, pc, arg0);
    }
}

static checker_t *cur_checker;
void user_mode_terminated(int ret) {
  cur_checker->checker_regs.regs[0] = ret;
  sys_switchto(&cur_checker->checker_regs);
}

int run_in_user_mode(int (*B)(struct checker *), struct checker *c) {
  enum { N=8192 };
  static uint64_t stack[N];

  uint32_t cpsr_cur = cpsr_get();
  assert(mode_get(cpsr_cur) != USER_MODE);
  uint32_t cpsr = mode_set(cpsr_cur, USER_MODE);

  cur_checker = c;
  if (!cur_checker->b_yielded) {
    cur_checker->b_regs = (regs_t){ 
      .regs[REGS_PC] = (uint32_t)B,
      .regs[REGS_R0] = (uint32_t)c,
      .regs[REGS_SP] = (uint32_t)&stack[N],
      .regs[REGS_CPSR] = cpsr,
      .regs[REGS_LR] = (uint32_t)user_mode_terminated,
    };
  }
  cur_checker->b_yielded = 0;
  switchto_cswitch(&cur_checker->checker_regs, &cur_checker->b_regs);

  // We must have returned from user_mode_terminated
  return cur_checker->checker_regs.regs[0];
}

// called on each single step exception: 
// if we have run switch_on_inst_n instructions then:
//  1. run B().
//     - if B() succeeds, then run the rest of A() to completion.  
//     - if B() returns 0, it couldn't complete w current state:
//       resume A() and switch on next instruction.
static void A_terminated(uint32_t ret);
void single_step_handler_full(regs_t *r) {
    if(!brkpt_fault_p())
        panic("impossible: should get no other faults\n");

    uint32_t pc = r->regs[15];
    uint32_t n = ++checker->inst_count;

    // output("single-step handler: inst=%d: A:pc=%x\n", n,pc);
    
    // the weird way single step works: run the instruction at 
    // address <pc>, by setting up a mismatch fault for any other
    // pc value.
    if (checker->interleaving_p) {
      if (n >= checker->switch_on_inst_n) {
        brkpt_mismatch_stop();
        int ret = run_in_user_mode(checker->B, (struct checker*)checker);
        if (!checker->b_yielded && ret)
          checker->switched_p = 1;
        else
          brkpt_mismatch_set(pc);
      } else if (pc == (uint32_t)A_terminated) {
        // If we reached the end of A here, this must mean that B has not been
        // called yet, since single step wasn't disabled, so make sure to call
        // B here
        brkpt_mismatch_stop();
        run_in_user_mode(checker->B, (struct checker*)checker);
      } else {
        brkpt_mismatch_set(pc);
      }
    } else if (pc == (uint32_t)A_terminated) {
      brkpt_mismatch_stop();
    } else {
      brkpt_mismatch_set(pc);
    }
    switchto(r);
}

enum {
  LDREX = 0xe1900f9f,
  STREX = 0xe1800f90,
};

cp_asm(dfsr, p15, 0, c5, c0, 0)

void data_abort_handler_full(regs_t *r) {
  uint32_t pc = r->regs[REGS_PC];

  // DEBUG
  output("data abort at pc=%x\n", pc);
  uint32_t stat = dfsr_get();
  output("fault reason: (bit 10) %b, (bits 3-0) %b\n",
         (stat >> 10) & 1, stat & 0xf);

  uint32_t inst = *((uint32_t*)pc);
  static uint32_t prev_addr, has_outstanding_addr = 0;
  if ((inst & LDREX) == LDREX) {
    uint32_t rt = (inst >> 12) & 0xf;
    uint32_t rn = (inst >> 16) & 0xf;
    prev_addr = r->regs[rn];

    // DEBUG
    output("running instruction: ldrex r%d, [r%d]\n", rt, rn);
    output("address: %x\n", prev_addr);

    has_outstanding_addr = 1;
    r->regs[rt] = *((uint32_t*)prev_addr);
  } else if ((inst & STREX) == STREX) {
    uint32_t rd = (inst >> 12) & 0xf;
    uint32_t rt = inst & 0xf;
    uint32_t rn = (inst >> 16) & 0xf;
    uint32_t addr = r->regs[rn];

    // DEBUG
    output("running instruction: strex r%d, r%d, [r%d]\n", rd, rt, rn);
    output("address: %x\n", addr);

    if (has_outstanding_addr && addr == prev_addr) {
      *((uint32_t*)addr) = r->regs[rt];
      has_outstanding_addr = 0;
      r->regs[rd] = 0;
    } else {
      r->regs[rd] = 1;
    }
  } else {
    panic("undefined instruction %x at pc=%x\n", inst, pc);
  }
  r->regs[REGS_PC] += 4;
}

// registers saved when we started.
static regs_t start_regs;

// this is called when A() returns: assumes you are at user level.
// switch back to <start_regs>
static void A_terminated(uint32_t ret) {
    uint32_t cpsr = mode_get(cpsr_get());
    if(cpsr != USER_MODE)
        panic("should be at USER, at <%s> mode\n", mode_str(cpsr));

    start_regs.regs[0] = ret;
    sys_switchto(&start_regs);
}

// see lab 14 from 140e
static uint32_t run_A_at_userlevel(checker_t *c) {
    // 1. get current cpsr and change mode to USER_MODE.
    // this will preserve any interrupt flags etc.
    uint32_t cpsr_cur = cpsr_get();
    assert(mode_get(cpsr_cur) == SUPER_MODE);
    uint32_t cpsr_A = mode_set(cpsr_cur, USER_MODE);

    // 2. setup the registers.

    // allocate stack on our main stack.  grows down.
    enum { N=8192 };
    uint64_t stack[N];

    // see <switchto.h>
    regs_t r = { 
        // start running A.
        .regs[REGS_PC] = (uint32_t)c->A,
        // the first argument to A()
        .regs[REGS_R0] = (uint32_t)c,

        // stack pointer register
        .regs[REGS_SP] = (uint32_t)&stack[N],
        // the cpsr to use when running A()
        .regs[REGS_CPSR] = cpsr_A,

        // hack: setup registers so that when A() is done 
        // and returns will jump to <A_terminated> by 
        // setting the LR register to <A_terminated>.
        .regs[REGS_LR] = (uint32_t)A_terminated,
    };

    // 3. turn on mismatching
    brkpt_mismatch_start();

    // 4. context switch to <r> and save current execution
    // state in <start_regs>
    switchto_cswitch(&start_regs, &r);

    // 5. At this point A() finished execution.   We got 
    // here from A_terminated()

    // 6. turn off mismatch (single step)
    brkpt_mismatch_stop();

    // 7. return back to the checking loop.
    return start_regs.regs[0];
}

// <c> has pointers to four routines:
//  1. A(), B(): the two routines to check.
//  2. init(): initialize A() B() state.
//  3. check(): called after A()B() and returns 1 if checks out.
int check(checker_t *c) {
    // install exception handlers for 
    // (1) system calls
    // (2) prefetch abort (single stepping exception is a type
    //     of prefetch fault)
    // 
    // install is idempotent if already there.
    full_except_install(0);
    full_except_set_syscall(syscall_handler_full);
    full_except_set_prefetch(single_step_handler_full);
    full_except_set_data_abort(data_abort_handler_full);

    void init_sp(void);
    init_sp();

    // show how to the interface works by testing
    // A(),B() sequentially multiple times.
    // 
    // if the code is non-deterministic, or the init / check
    // routines are busted, this can detect it.
    //
    // if AB commuted, we could also check BA but this won't be 
    // true in general.
#if 0
    for(int i = 0; i < 10; i++) {
        // 1.  initialize the state.
        c->init(c);     
        // 2. run A()
        c->A(c);
        // 3. run B()
        if(!c->B(c)) 
            panic("B should not fail\n");
        // 4. check that the state passes.
        if(!c->check(c))
            panic("check failed sequentially: code is broken\n");
    }
#endif

    // shows how to run code with single stepping: do the same sequential
    // checking but run A() in single step mode: 
    // should still pass (obviously)
    checker = c;
#if 0
    for(int i = 0; i < 10; i++) {
        c->init(c);
        c->interleaving_p = 0;
        run_A_at_userlevel(c);
        if(!c->B(c))
            panic("B should not fail\n");
        if(!c->check(c))
            panic("check failed sequentially: code is broken\n");
    }
#endif

    //******************************************************************
    // this is what you build: check that A(),B() code works
    // with one context switch for each possible instruction in
    // A()
    //
    // for(int i = 0; ; i++) {
    //      int switched_p = 0;
    //      c->init()
    //      run c->A() in single step for i instructions.
    //
    //      single step handler: 
    //          if this is the ith instruction
    //              call B().  
    //              if B() returned 1 (B() could run)
    //                  switched_p = 1;
    //                  finish A() without single stepping
    //              else
    //                  run A() one more another instruction and try B()
    // 
    //      checker:  when done running A(),B():
    //          c->check()
    //          if(!switched_p)
    //              break;
    //  }
    // 
    //  return 0 if there were errors.
    
#if 1
    for (int i = 1; ; ++i) {
      output("Switch at instruction %d\n", i);
      c->interleaving_p = 1;
      c->switched_p = 0;
      c->switch_on_inst_n = i;
      c->inst_count = 0;
      c->b_yielded = 0;
      c->init(c);
      run_A_at_userlevel(c);
      c->nerrors += !c->check(c);
      ++c->ntrials;
      if (!c->switched_p)
        break;
    }
#endif

    output("Number of trials: %d\n", c->ntrials);
    output("Number of errors: %d\n", c->nerrors);

    return c->nerrors == 0;
}
