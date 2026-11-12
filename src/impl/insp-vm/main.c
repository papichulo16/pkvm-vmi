#include <errno.h>
#include <unistd.h>

#include "kvm.h"
#include "vmm.h"
#include "print.h"

static int vmm_init(int argc, char **argv) {

	long kvm_fd;
	long vm_fd;
	int ret = 1;

	(void)argc;
	(void)argv;

	kvm_fd = kvm_open();

	if (kvm_fd < 0) {

		print_err("open /dev/kvm", -errno);
		goto out;
	}

	vm_fd = kvm_create_pvm((int)kvm_fd, 0);

	if (vm_fd < 0) {

		print_err("create protected vm", -errno);
		goto close_kvm;
	}

	print("protected vm fd ");
	print_hex(vm_fd);
	print("\n");

  if (vmm_init_pvm(kvm_fd, vm_fd)) {

		print_err("error in vmm init", 67);
    goto close_kvm;
  }

  vmm_run_pvm(kvm_fd, vm_fd);

	ret = 0;
	close((int)vm_fd);

close_kvm:
	close((int)kvm_fd);
out:
	return ret;
}

int main(int argc, char **argv) {

	return vmm_init(argc, argv);
}

