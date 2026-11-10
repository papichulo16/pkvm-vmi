#include "kvm.h"
#include "print.h"
#include "sys.h"

__asm__(".text\n"
	".global _start\n"
	"_start:\n"
	"	mov x0, sp\n"
	"	bl vmm_main\n"
	"	brk #0\n");

static int vmm_run(int argc, char **argv) {

	long kvm_fd;
	long vm_fd;
	int ret = 1;

	(void)argc;
	(void)argv;

	kvm_fd = kvm_open();

	if (sys_is_err(kvm_fd)) {

		print_err("open /dev/kvm", kvm_fd);
		goto out;
	}

	vm_fd = kvm_create_pvm((int)kvm_fd, 0);

	if (sys_is_err(vm_fd)) {

		print_err("create protected vm", vm_fd);
		goto close_kvm;
	}

	print("protected vm fd ");
	print_hex(vm_fd);
	print("\n");

	ret = 0;
	sys_close((int)vm_fd);

close_kvm:
	sys_close((int)kvm_fd);
out:
	return ret;
}

void __attribute__((noreturn, used)) vmm_main(long *sp) {

	int argc = (int)sp[0];
	char **argv = (char **)(sp + 1);

	sys_exit(vmm_run(argc, argv));
}
