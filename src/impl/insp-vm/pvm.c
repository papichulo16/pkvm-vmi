
__asm__(
  ".text\n"
	".global _start\n"
	"reset:\n"
	"	b boot_start\n"
);

void arm64_disable_dcache(void) {
    unsigned long sctlr;

    __asm__ __volatile__(
        "mrs %[sctlr], sctlr_el1\n\t"
        "bic %[sctlr], %[sctlr], #(1 << 2)\n\t"
        "msr sctlr_el1, %[sctlr]\n\t"
        "dsb sy\n\t"
        "isb\n\t"
        : [sctlr] "=r" (sctlr)
        :
        : "memory"
    );
}

void __attribute__((noreturn, used)) boot_start() {

  static volatile int* mmio = (int *) 0x67000000;
  int val;

  arm64_disable_dcache();

  for (int i = 0; i < 10; i++)
    *mmio = i;

  val = *mmio;

  while (1);
}

