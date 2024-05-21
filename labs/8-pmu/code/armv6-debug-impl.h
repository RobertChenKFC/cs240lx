#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__

/*************************************************************************
 * all the different assembly routines.
 */
#include "asm-helpers.h"

#if 0
// all we need for IMB at the moment: prefetch flush.
static inline void prefetch_flush(void) {
    unsigned r = 0;
    asm volatile ("mcr p15, 0, %0, c7, c5, 4" :: "r" (r));
}
#endif

// turn <x> into a string
#define MK_STR(x) #x

// define a general co-processor inline assembly routine to set the value.
// from manual: must prefetch-flush after each set.
#define coproc_mk_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline void c ## coproc ## _ ## fn_name ## _set(uint32_t v) {                    \
        asm volatile ("mcr " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) :: "r" (v));               \
        prefetch_flush();                                               \
    }

#define coproc_mk_get(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline uint32_t c ## coproc ## _ ## fn_name ## _get(void) {                      \
        uint32_t ret=0;                                                   \
        asm volatile ("mrc " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) : "=r" (ret));             \
        return ret;                                                     \
    }


// make both get and set methods.
#define coproc_mk(fn, coproc, opcode_1, Crn, Crm, opcode_2)     \
    coproc_mk_set(fn, coproc, opcode_1, Crn, Crm, opcode_2)        \
    coproc_mk_get(fn, coproc, opcode_1, Crn, Crm, opcode_2) 

// produces p14_brv_get and p14_brv_set
// coproc_mk(brv, p14, 0, c0, crm, op2)

/*******************************************************************************
 * debug support.
 */
#include "libc/helper-macros.h"     // to check the debug layout.
#include "libc/bit-support.h"           // bit_* and bits_* routines.


// 13-5
struct debug_id {
                                // lower bit pos : upper bit pos [inclusive]
                                // see 0-example-debug.c for how to use macros
                                // to check bitposition and size.  very very easy
                                // to mess up: you should always do.
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register
                                //           pairs+1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// Get the debug id register
static inline uint32_t cp14_debug_id_get(void) {
    // the documents seem to imply the general purpose register 
    // SBZ ("should be zero") so we clear it first.
    uint32_t ret = 0;

    asm volatile ("mrc p14, 0, %0, c0, c0, 0" : "=r"(ret));
    return ret;
}

// This macro invocation creates a routine called cp14_debug_id_macro
// that is equivalant to <cp14_debug_id_get>
//
// you can see this by adding "-E" to the gcc compile line and inspecting
// the output.
coproc_mk_get(debug_id_macro, p14, 0, c0, c0, 0)

// enable the debug coproc
static inline void cp14_enable(void);

// get the cp14 status register.
static inline uint32_t cp14_status_get(void);
// set the cp14 status register.
static inline void cp14_status_set(uint32_t status);

#if 0
static inline uint32_t cp15_dfsr_get(void);
static inline uint32_t cp15_ifar_get(void);
static inline uint32_t cp15_ifsr_get(void);
static inline uint32_t cp14_dscr_get(void);
#endif

//**********************************************************************
// all your code should go here.  implementation of the debug interface.

// example of how to define get and set for status registers
coproc_mk(status, p14, 0, c0, c1, 0)

// you'll need to define these and a bunch of other routines.
coproc_mk(dfsr, p15, 0, c5, c0, 0)
coproc_mk(ifar, p15, 0, c6, c0, 2)
coproc_mk(ifsr, p15, 0, c5, c0, 1)
coproc_mk(dscr, p14, 0, c0, c1, 0)
coproc_mk(wcr0, p14, 0, c0, c0, 7)

coproc_mk(wvr0, p14, 0, c0, c0, 6)
coproc_mk(bcr0, p14, 0, c0, c0, 5)
coproc_mk(bvr0, p14, 0, c0, c0, 4)


// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
enum {
  DSCR_MNTR_DBG_ENABLE = 15, // arm1176-ch13-debug.pdf, p.9, Table 13-4
  DSCR_MODE_SELECT = 14, // arm1176-ch13-debug.pdf, p.9, Table 13-4
  DSCR_DBG_ENTRY = 2, // arm1176-ch13-debug.pdf, p.11, Table 13-4
  DSCR_DBG_ENTRY_BITS = 0b1111, // arm1176-ch13-debug.pdf, p.11, Table 13-4
  DSCR_BKPT_OCCUR = 0b0001, // arm1176-ch13-debug.pdf, p.11, Table 13-4
  DSCR_WCPT_OCCUR = 0b0010, // arm1176-ch13-debug.pdf, p.11, Table 13-4
  DSCR_CORE_HALTED = 0, // arm1176-ch13-debug.pdf, p.11, Table 13-4
};
static inline int cp14_is_enabled(void) {
  uint32_t status = cp14_dscr_get();
  if ((status >> DSCR_MODE_SELECT) & 1)
    return 0;
  return (status >> DSCR_MNTR_DBG_ENABLE) & 1;
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return?
    if(cp14_is_enabled())
      return;

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t status = cp14_dscr_get();
    status |= 1 << DSCR_MNTR_DBG_ENABLE;
    status &= ~(1 << DSCR_MODE_SELECT);
    cp14_dscr_set(status);

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    uint32_t status = cp14_dscr_get();
    status &= ~(1 << DSCR_MNTR_DBG_ENABLE);
    status |= 1 << DSCR_MODE_SELECT;
    cp14_dscr_set(status);

    assert(!cp14_is_enabled());
}


enum {
  BCR_MEANING = 21, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MEANING_BITS = 0b11, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MATCH_IMVA = 0b00, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MISMATCH_IMVA = 0b10, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_LINKING = 20, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_SECURE = 14, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MATCH_SECURE_NSECURE = 0b00, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_BYTE_ADDR_SELECT = 5, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MATCH_ADDR_ADD0 = 0b0001, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_MATCH_ADDR_ALL = 0b1111, // arm1176-ch13-debug.pdf, p.18, Table 13-11
  BCR_SUPER = 1, // arm1176-ch13-debug.pdf, p.19, Table 13-11
  BCR_SUPER_EITHER = 0b11, // arm1176-ch13-debug.pdf, p.19, Table 13-11
  BCR_BKPT_ENABLE = 0, // arm1176-ch13-debug.pdf, p.19, Table 13-11
};
static inline int cp14_bcr0_is_enabled(void) {
  uint32_t status = cp14_bcr0_get();
  return (status >> BCR_BKPT_ENABLE) & 1;
}
static inline void cp14_bcr0_enable_match_kind(uint32_t match_kind) {
  uint32_t status = cp14_bcr0_get();

  // set breakpoint to match on match_kind
  status &= ~(0b11 << BCR_MEANING);
  status |= match_kind << BCR_MEANING;
  // set breakpoint to disable linking
  status &= ~(1 << BCR_LINKING);
  // set breakpoint to match in secure and not secure world
  status &= ~(0b11 << BCR_SECURE);
  status |= BCR_MATCH_SECURE_NSECURE << BCR_SECURE;
  // set breakpoint to match bvr+any
  status &= ~(0b1111 << BCR_BYTE_ADDR_SELECT);
  status |= BCR_MATCH_ADDR_ALL << BCR_BYTE_ADDR_SELECT;
  // set breakpoint to match both privileged and user access
  status &= ~(0b11 << BCR_SUPER);
  status |= BCR_SUPER_EITHER << BCR_SUPER;
  // enable breakpoint
  status |= 1 << BCR_BKPT_ENABLE;

  cp14_bcr0_set(status);
}
static inline void cp14_bcr0_enable(void) {
  cp14_bcr0_enable_match_kind(BCR_MATCH_IMVA);
}
static inline void cp14_bcr0_disable(void) {
  uint32_t status = cp14_bcr0_get();

  // disable breakpoint
  status &= ~(1 << BCR_BKPT_ENABLE);

  cp14_bcr0_set(status);
}

// was this a brkpt fault?
enum {
  IFSR_STATUS = 0, // arm1176-fault-regs.pdf, p.4, Table 3-64
  IFSR_STATUS_BITS = 0b1111, // arm1176-fault-regs.pdf, p.4, Table 3-64
  IFSR_INST_DBG_FAULT = 0b0010, // arm1176-fault-regs.pdf, p.4, Table 3-64
};
static inline int was_brkpt_fault(void) {
  // use IFSR and then DSCR
  uint32_t status = cp15_ifsr_get();
  if (((status >> IFSR_STATUS) & IFSR_STATUS_BITS) != IFSR_INST_DBG_FAULT)
    return 0;
  status = cp14_dscr_get();
  return ((status >> DSCR_DBG_ENTRY) & DSCR_DBG_ENTRY_BITS) == DSCR_BKPT_OCCUR;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
enum {
  DFSR_RW = 11, // arm1176-fault-regs.pdf, p.1, Table 3-61
  DFSR_RW_RD = 0, // arm1176-fault-regs.pdf, p.1, Table 3-61
  DFSR_RW_WR = 1, // arm1176-fault-regs.pdf, p.1, Table 3-61
  DFSR_STATUS = 0, // arm1176-fault-regs.pdf, p.2, Table 3-61
  DFSR_STATUS_BITS = 0b1111, // arm1176-fault-regs.pdf, p.2, Table 3-61
  DFSR_INST_DBG_FAULT = 0b0010, // arm1176-fault-regs.pdf, p.2, Table 3-61
};
static inline int was_watchpt_fault(void) {
  // use DFSR then DSCR
  uint32_t status = cp15_dfsr_get();
  if (((status >> DFSR_STATUS) & DFSR_STATUS_BITS) != DFSR_INST_DBG_FAULT)
    return 0;
  status = cp14_dscr_get();
  return ((status >> DSCR_DBG_ENTRY) & DSCR_DBG_ENTRY_BITS) == DSCR_WCPT_OCCUR;
}

enum {
  WCR_LINKING = 20, // arm1176-ch13-debug.pdf, p.21, Table 13-16
  WCR_SECURE = 14, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_MATCH_SECURE_NSECURE = 0b00, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_BYTE_ADDR_SELECT = 5, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_LD_ST = 3, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_MATCH_LD_ST = 0b11, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_SUPER = 1, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_SUPER_EITHER = 0b11, // arm1176-ch13-debug.pdf, p.22, Table 13-16
  WCR_WCPT_ENABLE = 0, // arm1176-ch13-debug.pdf, p.22, Table 13-16
};
static inline int cp14_wcr0_is_enabled(void) {
  uint32_t status = cp14_wcr0_get();
  return (status >> WCR_WCPT_ENABLE) & 1;
}
static inline void cp14_wcr0_enable_byte_offset(uint32_t byte_offset) {
  uint32_t status = cp14_wcr0_get();

  // set watchpoint to disable linking
  status &= ~(1 << WCR_LINKING);
  // set watchpoint to match in secure and not secure world
  status &= ~(0b11 << WCR_SECURE);
  status |= WCR_MATCH_SECURE_NSECURE << WCR_SECURE;
  // set watchpoint to match wvr+byte_offset
  status &= ~(0b1111 << WCR_BYTE_ADDR_SELECT);
  status |= (1 << byte_offset) << WCR_BYTE_ADDR_SELECT;
  // set watchpoint to match both load and store
  status &= ~(0b11 << WCR_LD_ST);
  status |= WCR_MATCH_LD_ST << WCR_LD_ST;
  // set watchpoint to match both privileged and user access
  status &= ~(0b11 << WCR_SUPER);
  status |= WCR_SUPER_EITHER << WCR_SUPER;
  // enable watchpoint
  status |= 1 << WCR_WCPT_ENABLE;

  cp14_wcr0_set(status);
}
static inline void cp14_wcr0_enable(void) {
  cp14_wcr0_enable_byte_offset(0);
}
static inline void cp14_wcr0_disable(void) {
  uint32_t status = cp14_wcr0_get();
  status &= ~(1 << WCR_WCPT_ENABLE);
  cp14_wcr0_set(status);
}

// Get watchpoint fault using WFAR
coproc_mk(wfar, p14, 0, c0, c6, 0)
static inline uint32_t watchpt_fault_pc(void) {
  // arm1176-ch13-debug.pdf, p.12, 13.3.5
  return cp14_wfar_get() - 0x8;
}
    
#endif
