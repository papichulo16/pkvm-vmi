
__asm__(
  ".text\n"
	".global _start\n"
	"reset:\n"
	"	b boot_start\n"
);

void __attribute__((noreturn, used)) boot_start() {

  static int* mmio = (int *) 0x67000000;
  *mmio = 3;

  while(1);
}

