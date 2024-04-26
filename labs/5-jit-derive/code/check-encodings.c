#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include "libunix.h"
#include <unistd.h>
#include "code-gen.h"
#include "armv6-insts.h"

/*
 *  1. emits <insts> into a temporary file.
 *  2. compiles it.
 *  3. reads back in.
 *  4. returns pointer to it.
 */
uint32_t *insts_emit(unsigned *nbytes, const char *insts) {
  // check libunix.h --- create_file, write_exact, run_system, read_file.
  int fd = create_file("test.s");
  char buf[4096];
  sprintf(buf, "%s\n", insts);
  write_exact(fd, buf, strlen(buf));
  close(fd);
  run_system("arm-none-eabi-as test.s -o temp1 && arm-none-eabi-objcopy "
             "-O binary temp1 temp2");
  return read_file(nbytes, "temp2");
}

/*
 * a cross-checking hack that uses the native GNU compiler/assembler to 
 * check our instruction encodings.
 *  1. compiles <insts>
 *  2. compares <code,nbytes> to it for equivalance.
 *  3. prints out a useful error message if it did not succeed!!
 */
void insts_check(char *insts, uint32_t *code, unsigned nbytes) {
    // make sure you print out something useful on mismatch!
    uint32_t correct_nbytes;
    uint32_t *correct_code = insts_emit(&correct_nbytes, insts);
    if (correct_nbytes != nbytes) {
      panic("Expected %d bytes for instruction %s, got %d bytes instead\n",
            correct_nbytes, insts, nbytes);
    } else {
      for (int i = 0; i < correct_nbytes / 4; ++i) {
        if (correct_code[i] != code[i]) {
          panic("Expected instruction %d of %s be %x, got %x instead\n",
                i, insts, correct_code[i], code[i]);
        }
      }
    }
}

// check a single instruction.
void check_one_inst(char *insts, uint32_t inst) {
    return insts_check(insts, &inst, 4);
}

// helper function to make reverse engineering instructions a bit easier.
void insts_print(char *insts) {
    // emit <insts>
    unsigned gen_nbytes;
    uint32_t *gen = insts_emit(&gen_nbytes, insts);

    // print the result.
    output("getting encoding for: < %20s >\t= [", insts);
    unsigned n = gen_nbytes / 4;
    for(int i = 0; i < n; i++)
         output(" 0x%x ", gen[i]);
    output("]\n");
}

uint32_t emit_one_inst(const char *inst_str) {
  uint32_t n;
  uint32_t *c = insts_emit(&n, inst_str);
  assert(n == 4);
  uint32_t ret = *c;
  free(c);
  return ret;
}

// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_rrr(const char *op, const char *d, const char *s1, const char *s2) {
  char buf[1024];
  sprintf(buf, "%s %s, %s, %s", op, d, s1, s2);
  return emit_one_inst(buf);
}

uint32_t calc_differing_bits(
    const char *opcode, const char *fmt, const char *operands[],
    unsigned *operand_offset, unsigned *op) {
  static uint32_t nop = 0;
  if (!nop)
    nop = emit_one_inst("nop");

  uint32_t always_0 = ~0, always_1 = ~0, u;
  // compute any bits that changed as we vary each register.
  for (int i = 0; operands[i]; ++i) {
      char buf[4096], inst[4096];
      sprintf(buf, fmt, operands[i]);
      sprintf(inst, buf, opcode);

      uint32_t n;
      uint32_t *c = insts_emit(&n, inst);
      for (int j = 0; j < n; ++j) {
        // Assuming that the first instruction that is not a nop is what we
        // care about
        if (c[j] != nop) {
          u = c[j];
          break;
        }
      }
      assert(u);
      free(c);

      // if a bit is always 0 then it will be 1 in always_0
      always_0 &= ~u;

      // if a bit is always 1 it will be 1 in always_1, otherwise 0
      always_1 &= u;
  }

  if(always_0 & always_1) 
      panic("impossible overlap: always_0 = %x, always_1 %x\n", 
          always_0, always_1);

  // bits that never changed
  uint32_t never_changed = always_0 | always_1;
  // bits that changed: these are the register bits.
  uint32_t changed = ~never_changed;

  // output("register dst are bits set in: %x\n", changed);

  // find the offset.  we assume register bits are contig and within 0xf
  /*
  unsigned d_off = ffs(changed);
  
  // check that bits are contig and at most 4 bits are set.
  if(((changed >> d_off) & ~0xf) != 0)
      panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);
  */

  *operand_offset = ffs(changed) - 1;
  *op = ~changed & u;

  return changed;
}

// overly-specific.  some assumptions:
//  1. each register is encoded with its number (r0 = 0, r1 = 1)
//  2. related: all register fields are contiguous.
//
// NOTE: you should probably change this so you can emit all instructions 
// all at once, read in, and then solve all at once.
//
// For lab:
//  1. complete this code so that it solves for the other registers.
//  2. refactor so that you can reused the solving code vs cut and pasting it.
//  3. extend system_* so that it returns an error.
//  4. emit code to check that the derived encoding is correct.
//  5. emit if statements to checks for illegal registers (those not in <src1>,
//    <src2>, <dst>).
void derive_op_rrr(const char *name, const char *opcode, 
        const char **dst, const char **src1, const char **src2) {
    unsigned d_off = 0, src1_off = 0, src2_off = 0, op;
    calc_differing_bits(opcode, "%%s %s, r0, r0", dst, &d_off, &op);
    calc_differing_bits(opcode, "%%s r0, %s, r0", src1, &src1_off, &op);
    calc_differing_bits(opcode, "%%s r0, r0, %s", src2, &src2_off, &op);

    // emit: NOTE: obviously, currently <src1_off>, <src2_off> are not 
    // defined (so solve for them) and opcode needs to be refined more.
    output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
    output("    return 0x%x | (dst << %d) | (src1 << %d) | (src2 << %d);\n",
                op,
                d_off,
                src1_off,
                src2_off);
    output("}\n");
}

void derive_branch(const char *name, const char *opcode) {
  enum { NUM_OFFSETS = 21 };
  char buf[NUM_OFFSETS][4096];
  const char *insts[NUM_OFFSETS + 1];
  for (int i = 0; i < NUM_OFFSETS; ++i) {
    int offset = (i - (NUM_OFFSETS - 1) / 2) * 4;
    char *inst = buf[i];
    inst[0] = 0;
    if (offset <= 0)
      strcat(inst, "label: ");
    for (int i = offset; i < 0; i += 4)
      strcat(inst, "nop; ");
    strcat(inst, "%s label; ");
    for (int i = 4; i < offset; i += 4)
      strcat(inst, "nop; ");
    if (offset > 0)
      strcat(inst, "label: ");
    insts[i] = inst;
  }
  insts[NUM_OFFSETS] = NULL;

  unsigned b_off, op;
  unsigned b_mask = calc_differing_bits(opcode, "%s", insts, &b_off, &op);

  output("static int %s(uint32_t src_addr, uint32_t target_addr) {\n", name);
  output("    uint32_t off = (target_addr - src_addr - 8) / 4;\n");
  output("    return 0x%x | ((off << %d) & 0x%x);\n", op, b_off, b_mask);
  output("}\n");
}

void derive_branch_reg(const char *name, const char *opcode, const char **dst) {
  unsigned d_off = 0, op;
  calc_differing_bits(opcode, "%%s %s", dst, &d_off, &op);

  output("static int %s(uint32_t dst) {\n", name);
  output("    return 0x%x | (dst << %d);\n", op, d_off);
  output("}\n");
}

void derive_mem_op(const char *name, const char *opcode, const char **dst,
    const char **src, const char **pos_offsets, const char **neg_offsets) {

  unsigned d_off, src_off, pos_offset_off, neg_offset_off, op, pos_op, neg_op; 
  calc_differing_bits(opcode, "%%s %s, [r0]", dst, &d_off, &op);
  calc_differing_bits(opcode, "%%s r0, [%s]", src, &src_off, &op);
  unsigned offset_mask = calc_differing_bits(
      opcode, "%%s r0, [r0, #%s]", pos_offsets, &pos_offset_off, &pos_op);
  offset_mask &= calc_differing_bits(
      opcode, "%%s r0, [r0, #%s]", neg_offsets, &neg_offset_off, &neg_op);

  output("static int %s(uint32_t dst, uint32_t src, int offset) {\n", name);
  output("    if (offset >= 0)\n");
  output("        return 0x%x | (dst << %d) | (src << %d) | "
         "((offset << %d) & 0x%x);\n",
         pos_op, d_off, src_off, pos_offset_off, offset_mask);
  output("    else\n");
  output("        return 0x%x | (dst << %d) | (src << %d) | "
         "((-offset << %d) & 0x%x);\n",
         neg_op, d_off, src_off, neg_offset_off, offset_mask);
  output("}\n");
}

#include "generated-encodings.h"

/*
 * 1. we start by using the compiler / assembler tool chain to get / check
 *    instruction encodings.  this is sleazy and low-rent.   however, it 
 *    lets us get quick and dirty results, removing a bunch of the mystery.
 *
 * 2. after doing so we encode things "the right way" by using the armv6
 *    manual (esp chapters a3,a4,a5).  this lets you see how things are 
 *    put together.  but it is tedious.
 *
 * 3. to side-step tedium we use a variant of (1) to reverse engineer 
 *    the result.
 *
 *    we are only doing a small number of instructions today to get checked off
 *    (you, of course, are more than welcome to do a very thorough set) and focus
 *    on getting each method running from beginning to end.
 *
 * 4. then extend to a more thorough set of instructions: branches, loading
 *    a 32-bit constant, function calls.
 *
 * 5. use (4) to make a simple object oriented interface setup.
 *    you'll need: 
 *      - loads of 32-bit immediates
 *      - able to push onto a stack.
 *      - able to do a non-linking function call.
 */
int main(void) {
    // part 1: implement the code to do this.
    output("-----------------------------------------\n");
    output("part1: checking: correctly generating assembly.\n");
    insts_print("add r0, r0, r1");
    insts_print("bx lr");
    insts_print("mov r0, #1");
    insts_print("nop");
    output("\n");
    output("success!\n");

    // part 2: implement the code so these checks pass.
    // these should all pass.
    output("\n-----------------------------------------\n");
    output("part 2: checking we correctly compare asm to machine code.\n");
    check_one_inst("add r0, r0, r1", 0xe0800001);
    check_one_inst("bx lr", 0xe12fff1e);
    check_one_inst("mov r0, #1", 0xe3a00001);
    check_one_inst("nop", 0xe1a00000);
    output("success!\n");

    // part 3: check that you can correctly encode an add instruction.
    output("\n-----------------------------------------\n");
    output("part3: checking that we can generate an <add> by hand\n");
    check_one_inst("add r0, r1, r2", handcoded_arm_add(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r3, r4, r5", handcoded_arm_add(arm_r3, arm_r4, arm_r5));
    check_one_inst("add r6, r7, r8", handcoded_arm_add(arm_r6, arm_r7, arm_r8));
    check_one_inst("add r9, r10, r11", handcoded_arm_add(arm_r9, arm_r10, arm_r11));
    check_one_inst("add r12, r13, r14", handcoded_arm_add(arm_r12, arm_r13, arm_r14));
    check_one_inst("add r15, r7, r3", handcoded_arm_add(arm_r15, arm_r7, arm_r3));
    output("success!\n");

    // part 4: implement the code so it will derive the add instruction.
    output("\n-----------------------------------------\n");
    output("part4: checking that we can reverse engineer an <add>\n");

    const char *all_regs[] = {
                "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                0 
    };
    // XXX: should probably pass a bitmask in instead.
    derive_op_rrr("arm_add", "add", all_regs,all_regs,all_regs);
    // output("did something: now use the generated code in the checks above!\n");
    // get encodings for other instructions, loads, stores, branches, etc.
    derive_branch("arm_b", "b");
    derive_branch("arm_bl", "bl");
    derive_branch_reg("arm_bx", "bx", all_regs);
    derive_branch_reg("arm_blx", "blx", all_regs);
    const char *pos_offsets[] = { "0xfff", "0xff", "0xf", "0", NULL };
    const char *neg_offsets[] = { "-0xfff", "-0xff", "-0xf", "0", NULL };
    derive_mem_op(
        "arm_ldr", "ldr", all_regs, all_regs, pos_offsets, neg_offsets);
    derive_mem_op(
        "arm_str", "str", all_regs, all_regs, pos_offsets, neg_offsets);

    // part 5: cross check the generated code
    output("\n-----------------------------------------\n");
    output("part5: cross check the generated code\n");
    check_one_inst("b label; label: ", arm_b(0,4));
    check_one_inst("bl label; label: ", arm_bl(0,4));
    check_one_inst("label: b label; ", arm_b(0,0));
    check_one_inst("label: bl label; ", arm_bl(0,0));
    check_one_inst("ldr r0, [pc,#0]", arm_ldr(arm_r0, arm_r15, 0));
    check_one_inst("ldr r0, [pc,#256]", arm_ldr(arm_r0, arm_r15, 256));
    check_one_inst("ldr r0, [pc,#-256]", arm_ldr(arm_r0, arm_r15, -256));

    return 0;
}
