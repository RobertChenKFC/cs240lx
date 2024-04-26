// Automatically generated code. DO NOT MODIFY
static int arm_add(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0800000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_b(uint32_t src_addr, uint32_t target_addr) {
    uint32_t off = (target_addr - src_addr - 8) / 4;
    return 0xea000000 | ((off << 0) & 0xffffff);
}
static int arm_bl(uint32_t src_addr, uint32_t target_addr) {
    uint32_t off = (target_addr - src_addr - 8) / 4;
    return 0xeb000000 | ((off << 0) & 0xffffff);
}
static int arm_bx(uint32_t dst) {
    return 0xe12fff10 | (dst << 0);
}
static int arm_blx(uint32_t dst) {
    return 0xe12fff30 | (dst << 0);
}
static int arm_ldr(uint32_t dst, uint32_t src, int offset) {
    if (offset >= 0)
        return 0xe5900000 | (dst << 12) | (src << 16) | ((offset << 0) & 0xfff);
    else
        return 0xe5100000 | (dst << 12) | (src << 16) | ((-offset << 0) & 0xfff);
}
static int arm_str(uint32_t dst, uint32_t src, int offset) {
    if (offset >= 0)
        return 0xe5800000 | (dst << 12) | (src << 16) | ((offset << 0) & 0xfff);
    else
        return 0xe5000000 | (dst << 12) | (src << 16) | ((-offset << 0) & 0xfff);
}
