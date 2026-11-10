#include <asm/kvm_pkvm_module.h>
#include <nvhe/trap_handler.h>

#include "el2.h"
#include "pkvmi_args.h"


#define DESC_VALID     (1UL << 0)
#define DESC_TABLE     (1UL << 1)
#define DESC_ADDR_MASK GENMASK_ULL(47, 12)

bool el2_va_mapped(unsigned long va) {

  unsigned long va_bits = 64 - (read_sysreg(tcr_el2) & 0x3f);
  int shift = PAGE_SHIFT + 9 * ((va_bits - PAGE_SHIFT - 1) / 9);
  unsigned long* table = ops->hyp_va(read_sysreg(ttbr0_el2) & DESC_ADDR_MASK);
  unsigned long desc = table[(va >> shift) & 0x1ff];

  while ((desc & DESC_VALID) && (desc & DESC_TABLE) && shift > PAGE_SHIFT) {
    table = ops->hyp_va(desc & DESC_ADDR_MASK);
    shift -= 9;
    desc = table[(va >> shift) & 0x1ff];
  }

  return desc & DESC_VALID;
}
