#include <linux/fs.h>

#include "kvm.h"
#include "vmm.h"
#include "bridge.h"
#include "print.h"
#include "sys.h"

#define PVM_IMG_PATH "pvm.o"
#define PVM_IMG_SLOT 0
#define PVM_IMG_GPA 0x0
#define PVM_PAGE_SIZE 0x1000UL

#define PVM_STACK_SLOT PVM_IMG_SLOT+1
#define PVM_STACK_GPA 0xcd000000
#define PVM_STACK_SIZE 0x4000

struct vmm_device devices[] = {
  {
    .paddr = 0x67000000,
    .size = 0x1000,
    .read = bridge_device_read,
    .write = bridge_device_write
  }
};

static void vmm_handle_mmio_exit(struct kvm_run* run) {

  unsigned long gpa = run->mmio.phys_addr; 
  unsigned char* data = run->mmio.data;
  struct vmm_device* dev;

  for (int i = 0; i < (int) (sizeof(devices) / sizeof(struct vmm_device)); i++) {

    dev = &devices[i];

    if (!dev)
      continue;

    if(gpa < dev->paddr || gpa >= (dev->paddr + dev->size))
      continue;

    if (run->mmio.is_write && dev->write)
      dev->write(gpa, *(unsigned long *) data);
    else if (dev->read)
      dev->read(gpa);

    break;
  }
}

static int vmm_handle_vm_exit(struct kvm_run* run) {

  switch (run->exit_reason) {

    case KVM_EXIT_MMIO:
      vmm_handle_mmio_exit(run);
      break;

    case KVM_EXIT_HLT:
      print("exited via halt\n");
      return 1;

    case KVM_EXIT_FAIL_ENTRY:
      print("kvm hardware exit failure\n");
      return 1;
  }

  return 0;
}

static long pvm_init_mem(int kvm_fd, int vm_fd) {

  unsigned long size;
  unsigned long map_size;
  unsigned long done = 0;
  char *mem;
  char *stack;
  long fd;
  long ret;

  (void)kvm_fd;

  fd = sys_open(PVM_IMG_PATH, O_RDONLY);

  if (sys_is_err(fd)) {

    ret = fd;
    goto out;
  }

  size = sys_call(__NR_lseek, fd, 0, SEEK_END, 0, 0, 0);
  sys_call(__NR_lseek, fd, 0, SEEK_SET, 0, 0, 0);

  // rounded to the nearest page
  map_size = (size + PVM_PAGE_SIZE - 1) & ~(PVM_PAGE_SIZE - 1);
  mem = sys_mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  stack = sys_mmap(0, PVM_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (sys_mmap_failed(mem)) {

    ret = (long)mem;
    goto close_fd;
  }

  // write the file into the mmapped region
  while (done < size) {

    ret = sys_read(fd, mem + done, size - done);

    if (sys_is_err(ret))
      goto unmap;

    if (ret == 0) {

      ret = -EIO;
      goto unmap;
    }

    done += ret;
  }

  ret = kvm_set_mem(vm_fd, PVM_IMG_SLOT, PVM_IMG_GPA, map_size, mem);

  if (sys_is_err(ret))
    goto unmap;

  ret = kvm_set_mem(vm_fd, PVM_STACK_SLOT, PVM_STACK_GPA, PVM_STACK_SIZE, stack);

  if (sys_is_err(ret))
    goto unmap;

  goto close_fd;

unmap:
  sys_munmap(mem, map_size);
  sys_munmap(stack, PVM_STACK_SIZE);
close_fd:
  sys_close(fd);
out:
  return ret;
}

static int vmm_init_pvm_vcpu(int vm_fd) {

  int vcpu_fd = (int) kvm_create_vcpu(vm_fd, (unsigned long) 0);
  kvm_init_vcpu(vm_fd, vcpu_fd);

  struct kvm_regs regs = {
    .regs.pc = PVM_IMG_GPA,
    .regs.sp = PVM_STACK_GPA + PVM_STACK_SIZE,
  };
  sys_ioctl(vcpu_fd, KVM_SET_REGS, (unsigned long) &regs);

  return vcpu_fd;
}

static struct kvm_run* vmm_init_pvm_run_obj(int kvm_fd, int vcpu_fd) {

  long mmap_size = sys_ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  return (struct kvm_run *) sys_mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0);
}

int vmm_init_pvm(int kvm_fd, int vm_fd) {

  long ret = pvm_init_mem(kvm_fd, vm_fd);

  if (ret)
    print_err("load " PVM_IMG_PATH, ret);

  return (int)ret;
}

void vmm_run_pvm(int kvm_fd, int vm_fd) {

  int vcpu_fd = vmm_init_pvm_vcpu(vm_fd);
  struct kvm_run* run = vmm_init_pvm_run_obj(kvm_fd, vcpu_fd);

  print("running...\n");

  while(1) {

    kvm_run_vcpu(vcpu_fd);
    if (vmm_handle_vm_exit(run))
      return;
  }
}

