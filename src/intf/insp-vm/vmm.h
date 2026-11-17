#ifndef INSP_VMM_H
#define INSP_VMM_H

#include "user_msgq.h"

int vmm_init_pvm(int kvm_fd, int vm_fd);
void vmm_run_pvm(msgq_t* q, int kvm_fd, int vm_fd);

struct vmm_device {
  unsigned long paddr;
  size_t size;

  void (*read)(void* arg, unsigned long off);
  void (*write)(void* arg, unsigned long off, unsigned long val);
};

#endif

