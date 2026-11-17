#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "kvm.h"

long kvm_open(void) {

	long fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
	long ver;

	if (fd < 0)
		goto out;

	ver = ioctl(fd, KVM_GET_API_VERSION, 0);

	if (ver != KVM_API_VERSION) {
		close(fd);

		if (ver >= 0)
			errno = EINVAL;

		fd = -1;
	}

out:
	return fd;
}

long kvm_create_pvm(int kvm_fd, unsigned int ipa_bits) {

	unsigned long type = KVM_VM_TYPE_ARM_PROTECTED | KVM_VM_TYPE_ARM_IPA_SIZE(ipa_bits);
	long ret;

  /*
	ret = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_ARM_PROTECTED_VM);

	if (ret <= 0) {
		if (ret == 0)
			errno = EOPNOTSUPP;

		ret = -1;
		goto out;
	}
  */

	ret = ioctl(kvm_fd, KVM_CREATE_VM, type);

out:
	return ret;
}

long kvm_set_mem(int vm_fd, unsigned int slot, unsigned long gpa,
		 unsigned long size, void *host_virt) {

	struct kvm_userspace_memory_region region = {
		.slot = slot,
		.guest_phys_addr = gpa,
		.memory_size = size,
		.userspace_addr = (unsigned long)host_virt,
	};

	return ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, (unsigned long)&region);
}

long kvm_create_vcpu(int vm_fd, unsigned int id) {

	return ioctl(vm_fd, KVM_CREATE_VCPU, id);
}

long kvm_init_vcpu(int vm_fd, int vcpu_fd) {

	struct kvm_vcpu_init init = { 0 };
	long ret;

	ret = ioctl(vm_fd, KVM_ARM_PREFERRED_TARGET, (unsigned long)&init);

	if (ret)
		goto out;

	ret = ioctl(vcpu_fd, KVM_ARM_VCPU_INIT, (unsigned long)&init);

out:
	return ret;
}

struct kvm_run *kvm_map_run(int kvm_fd, int vcpu_fd, unsigned long *len) {

	long size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	void *run;

	if (size < 0) {
		run = MAP_FAILED;
		goto out;
	}

	run = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0);
	*len = size;

out:
	return run;
}

long kvm_run_vcpu(int vcpu_fd) {

	return ioctl(vcpu_fd, KVM_RUN, 0);
}

long kvm_set_reg(int vcpu_fd, unsigned long id, unsigned long val) {

	struct kvm_one_reg reg = {
		.id = id,
		.addr = (unsigned long)&val,
	};

	return ioctl(vcpu_fd, KVM_SET_ONE_REG, (unsigned long)&reg);
}

long kvm_get_reg(int vcpu_fd, unsigned long id, unsigned long *val) {

	struct kvm_one_reg reg = {
		.id = id,
		.addr = (unsigned long)val,
	};

	return ioctl(vcpu_fd, KVM_GET_ONE_REG, (unsigned long)&reg);
}

