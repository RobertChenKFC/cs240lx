#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__

enum {
    armv6_lr = 14,
    armv6_pc = 15,
    armv6_sp = 13,
    armv6_r0 = 0,

    op_mov = 0b1101,
    armv6_mvn = 0b1111,
    armv6_orr = 0b1100,
    op_mult = 0b0000,

    cond_always = 0b1110
};

// trivial wrapper for register values so type-checking works better.
typedef struct {
    uint8_t reg;
} reg_t;
static inline reg_t reg_mk(unsigned r) {
    if(r >= 16)
        panic("illegal reg %d\n", r);
    return (reg_t){ .reg = r };
}

// how do you add a negative? do a sub?
static inline uint32_t armv6_mov(reg_t rd, reg_t rn) {
    // could return an error.  could get rid of checks.
    //         I        opcode        | rd
    return cond_always << 28
        | op_mov << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}

static inline uint32_t 
armv6_mov_imm8_rot4(reg_t rd, uint32_t imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    return 0xe3a00000 | (rd.reg << 12) | (rot4 << 8) | imm8;
}
static inline uint32_t 
armv6_mov_imm8(reg_t rd, uint32_t imm8) {
    return armv6_mov_imm8_rot4(rd,imm8,0);
}

static inline uint32_t 
armv6_mvn_imm8(reg_t rd, uint32_t imm8) {
    todo("implement mvn\n");
}

static inline uint32_t 
armv6_bx(reg_t rd) {
  return 0xe12fff10 | rd.reg;
}

// use 8-bit immediate imm8, with a 4-bit rotation.
static inline uint32_t 
armv6_orr_imm8_rot4(reg_t rd, reg_t rn, unsigned imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    return 0xe3800000 | (rd.reg << 12) | (rot4 << 8) | imm8;
}

static inline uint32_t 
armv6_orr_imm8(reg_t rd, reg_t rn, unsigned imm8) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);

    todo("implement orr with immediate\n");
}

// a4-80
static inline uint32_t 
armv6_mult(reg_t rd, reg_t rm, reg_t rs) {
    todo("implement mult\n");
}


// load a word from memory[offset]
// ldr rd, [rn,#offset]
static inline uint32_t 
armv6_ldr_off12(reg_t rd, reg_t rn, int offset) {
    // a5-20
    uint32_t imm;
    uint32_t hdr;
    if (offset < 0) {
      imm = -offset;
      hdr = 0xe5100000;
    } else {
      imm = offset;
      hdr = 0xe5900000;
    }
    assert(imm <= 0xfff);
    return hdr | ((uint32_t)rn.reg << 16) | ((uint32_t)rd.reg << 12) | imm;
    // DEBUG
    /*
    reg_t r0 = reg_mk(0);
    return armv6_mov(r0, r0);
    */
}

/**********************************************************************
 * synthetic instructions.
 * 
 * these can result in multiple instructions generated so we have to 
 * pass in a location to store them into.
 */

static inline uint32_t *
armv6_load_imm32(uint32_t *code, reg_t rd, uint32_t imm32) {
  code[0] = armv6_ldr_off12(rd, reg_mk(armv6_pc), 0); // ldr rd, [pc, #8]
  code[1] = 0xea000000; // b pc+4
  code[2] = imm32;
  return code + 3;
}

// MLA (Multiply Accumulate) multiplies two signed or unsigned 32-bit
// values, and adds a third 32-bit value.
//
// a4-66: multiply accumulate.
//      rd = rm * rs + rn.
static inline uint32_t
armv6_mla(reg_t rd, reg_t rm, reg_t rs, reg_t rn) {    
  return 0xe0200090 | (rd.reg << 16) | (rn.reg << 12) | (rs.reg << 8) | rm.reg;
}

#endif
