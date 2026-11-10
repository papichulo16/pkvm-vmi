#ifndef INSP_VMM_H
#define INSP_VMM_H

int vmm_init_pvm(int kvm_fd, int vm_fd);
void vmm_run_pvm(int kvm_fd, int vm_fd);

#endif

