__asm__(".include \"pvm_asm.inc\"\n");

static void arm64_disable_dcache(void) {
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

void sync_handler(void* sp) {}
void irq_handler(void* sp) {}
void fiq_handler(void* sp) {}
void serror_handler(void* sp) {}

// https://wiki.osdev.org/AArch64_Exceptions
static void init_vbar_el1() {

  __asm__(
      "adr x0, vbase\n\t"
      "msr vbar_el1, x0\n\t"
      ::: "x0"
  );
}

static void init_gic() {
}

static void init_boot() {

  init_vbar_el1();
  init_gic();
} 

static void __attribute__((noreturn)) idle() {
  while (1) {
    __asm__("wfi");
  }
}

void __attribute__((noreturn, used)) boot_start() {

  static volatile int* mmio = (int *) 0x67000000;
  int val;

  arm64_disable_dcache();
  init_boot();

  for (int i = 0; i < 10; i++)
    *mmio = i;

  val = *mmio;

  idle();
}

