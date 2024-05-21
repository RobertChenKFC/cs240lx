#include "rpi.h"
#include "pt-vm.h"
#include "helper-macros.h"
#include "procmap.h"

// turn this off if you don't want all the debug output.
enum { verbose_p = 0 };
enum { OneMB = 1024*1024 };

vm_pt_t *vm_pt_alloc(unsigned n) {
    demand(n == 4096, we only handling a fully-populated page table right now);

    vm_pt_t *pt = 0;
    unsigned nbytes = 4096 * sizeof *pt;

    // allocate pt with n entries.
    // pt = staff_vm_pt_alloc(n);
    pt = kmalloc_aligned(nbytes, 1 << 14);

    demand(is_aligned_ptr(pt, 1<<14), must be 14-bit aligned!);
    return pt;
}

// allocate new page table and copy pt.
vm_pt_t *vm_dup(vm_pt_t *pt1) {
    vm_pt_t *pt2 = vm_pt_alloc(PT_LEVEL1_N);
    memcpy(pt2,pt1,PT_LEVEL1_N * sizeof *pt1);
    return pt2;
}

// same as pinned version: probably should check that
// the page table is set.
void vm_mmu_enable(void) {
    assert(!mmu_is_enabled());
    mmu_enable();
    assert(mmu_is_enabled());
}

// same as pinned version.
void vm_mmu_disable(void) {
    assert(mmu_is_enabled());
    mmu_disable();
    assert(!mmu_is_enabled());
}

// - set <pt,pid,asid> for an address space.
// - must be done before you switch into it!
// - mmu can be off or on.
void vm_mmu_switch(vm_pt_t *pt, uint32_t pid, uint32_t asid) {
    assert(pt);
    mmu_set_ctx(pid, asid, pt);
}

// just like pinned.
void vm_mmu_init(uint32_t domain_reg) {
    // initialize everything, after bootup.
    mmu_init();
    domain_access_ctrl_set(domain_reg);
}

// map the 1mb section starting at <va> to <pa>
// with memory attribute <attr>.
vm_pte_t *
vm_map_sec(vm_pt_t *pt, uint32_t va, uint32_t pa, pin_t attr) 
{
    assert(aligned(va, OneMB));
    assert(aligned(pa, OneMB));

    // today we just use 1mb.
    assert(attr.pagesize == PAGE_1MB);

    unsigned index = va >> 20;
    assert(index < PT_LEVEL1_N);

    // return staff_vm_map_sec(pt,va,pa,attr);
    unsigned first_level_table_idx = va >> 20;
    vm_pte_t *pte = &pt[first_level_table_idx];
    pte->tag = 0b10;
    // look at mem_attr.h: mem_attr_t and TEX_C_B
    pte->TEX = (attr.mem_attr >> 2) & 0b111;
    pte->C = (attr.mem_attr >> 1) & 1;
    pte->B = attr.mem_attr & 1;
    pte->XN = 0;
    pte->domain = attr.dom;
    pte->IMP = 0;
    // look at mem_attr.h: mem_perm_t
    pte->AP = attr.AP_perm & 0b11;
    pte->APX = (attr.AP_perm >> 2) & 1;
    pte->S = 0;
    pte->nG = !attr.G;
    pte->super = attr.pagesize == _16mb;
    pte->_sbz1 = 0;
    pte->sec_base_addr = pa >> 20;

    if(verbose_p)
        vm_pte_print(pt,pte);
    assert(pte);
    return pte;
}

/*
enum {
  PA_SUCCESS = 0 // arm1176-ch3-coproc.annot.pdf, p.81, Table 3-78
};
int tlb_contains_va(uint32_t *result, uint32_t va) {
    // 3-79
    assert(bits_get(va, 0,2) == 0);
    // DEBUG
    /// return staff_tlb_contains_va(result, va);
    xlate_kern_rd_set(va);
    uint32_t x = xlate_pa_get();
    *result = x & ~(0b1111111111);
    return !((x >> PA_SUCCESS) & 1);
}
cp_asm(xlate_pa,       p15, 0, c7,  c4, 0)
cp_asm(xlate_kern_rd,  p15, 0, c7,  c8, 0)
cp_asm(xlate_kern_wr,  p15, 0, c7,  c8, 1)
*/

// lookup 32-bit address va in pt and return the pte
// if it exists, 0 otherwise.
vm_pte_t * vm_lookup(vm_pt_t *pt, uint32_t va) {
    // return staff_vm_lookup(pt,va);

    // arm1176-vm.annot.pdf, p.46, Figure 6-11
    uint32_t first_level_table_idx = va >> 20;
    if (first_level_table_idx >= PT_LEVEL1_N)
      return NULL;
    vm_pte_t *pte = &pt[first_level_table_idx];
    // TODO: don't really know how to check that entry exists; assuming that
    //       entry is 0 if not set
    if (*((uint32_t*)pte) == 0)
      return NULL;
    return pte;
}

// manually translate <va> in page table <pt>
// - if doesn't exist, returns 0.
// - if does exist returns the corresponding physical
//   address in <pa>
//
// NOTE: we can't just return the result b/c page 0 could be mapped.
vm_pte_t *vm_xlate(uint32_t *pa, vm_pt_t *pt, uint32_t va) {
    // return staff_vm_xlate(pa,pt,va);

    vm_pte_t *pte = vm_lookup(pt, va);
    if (!pte)
      return NULL;
    // arm1176-vm.annot.pdf, p.46, Figure 6-11
    *pa = (pte->sec_base_addr << 20) | (va & 0xfffff);
    return pte;
}

// compute the default attribute for each type.
static inline pin_t attr_mk(pr_ent_t *e) {
    switch(e->type) {
    case MEM_DEVICE: 
        return pin_mk_device(e->dom);
    // kernel: currently everything is uncached.
    case MEM_RW:
        return pin_mk_global(e->dom, perm_rw_priv, MEM_uncached);
   case MEM_RO: 
        panic("not handling\n");
   default: 
        panic("unknown type: %d\n", e->type);
    }
}

// setup the initial kernel mapping.  This will mirror
//  static inline void procmap_pin_on(procmap_t *p) 
// in <15-pinned-vm/code/procmap.h>  but will call
// your vm_ routines, not pinned routines.
//
// if <enable_p>=1, should enable the MMU.  make sure
// you setup the page table and asid. use  
// kern_asid, and kern_pid.
vm_pt_t *vm_map_kernel(procmap_t *p, int enable_p) {
    // the asid and pid we start with.  
    //    shouldn't matter since kernel is global.
    enum { kern_asid = 1, kern_pid = 0x140e };


    // return staff_vm_map_kernel(p,enable_p);

    // compute all the domain permissions.
    vm_pt_t *pt = vm_pt_alloc(PT_LEVEL1_N);
    uint32_t d = dom_perm(p->dom_ids, DOM_client);
    vm_mmu_init(d);

    for(unsigned i = 0; i < p->n; i++) {
        pr_ent_t *e = &p->map[i];
        demand(e->nbytes == MB, "nbytes=%d\n", e->nbytes);
        pin_t g = attr_mk(e);
        vm_map_sec(pt, e->addr, e->addr, g);
    }

    debug("about to turn on mmu\n");

    // setup.
    vm_mmu_switch(pt, kern_pid, kern_asid);

    vm_mmu_enable();
    assert(mmu_is_enabled());
    debug("enabled!\n");

    // can only check this after MMU is on.
    debug("going to check entries are pinned\n");
    for(unsigned i = 0; i < p->n; i++) {
      if(!mmu_is_enabled())
        panic("XXX: i think we can only check existence w/ mmu enabled\n");

      uint32_t r, va = p->map[i].addr;
      if(vm_xlate(&r, pt, va)) {
          debug("success: page table contains %x, returned %x\n", va, r);
          assert(va == r);
      } else {
          panic("page table should have %x: returned %x [reason=%b]\n",
              va, r, bits_get(r,1,6));
      }
    }

    if (!enable_p)
      vm_mmu_disable();

    assert(pt);
    return pt;
}
