#ifndef INSP_KVM_H
#define INSP_KVM_H

#include <stddef.h>
#include <linux/kvm.h>

long kvm_open(void);
long kvm_create_pvm(int kvm_fd, unsigned int ipa_bits);
long kvm_set_mem(int vm_fd, unsigned int slot, unsigned long gpa,
		 unsigned long size, void *host_virt);

long kvm_create_vcpu(int vm_fd, unsigned int id);
long kvm_init_vcpu(int vm_fd, int vcpu_fd);
struct kvm_run *kvm_map_run(int kvm_fd, int vcpu_fd, unsigned long *len);
long kvm_run_vcpu(int vcpu_fd);

long kvm_set_reg(int vcpu_fd, unsigned long id, unsigned long val);
long kvm_get_reg(int vcpu_fd, unsigned long id, unsigned long *val);

#define KVM_CORE_REG(name) \
	(KVM_REG_ARM64 | KVM_REG_SIZE_U64 | KVM_REG_ARM_CORE | \
	 KVM_REG_ARM_CORE_REG(name))

#define KVM_REG_PC KVM_CORE_REG(regs.pc)
#define KVM_REG_PSTATE KVM_CORE_REG(regs.pstate)
#define KVM_REG_X(n) KVM_CORE_REG(regs.regs[n])

#endif

