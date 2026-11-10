#ifndef INSP_VMM_H
#define INSP_VMM_H

int vmm_init_pvm(int kvm_fd, int vm_fd);
void vmm_run_pvm(int kvm_fd, int vm_fd);

struct vmm_device {
  unsigned long paddr;
  size_t size;

  void (*read)(unsigned long off);
  void (*write)(unsigned long off, unsigned long val);
};

#endif

