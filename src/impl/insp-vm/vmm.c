#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "kvm.h"
#include "vmm.h"
#include "bridge.h"
#include "print.h"
#include "user_msgq.h"

#define PVM_IMG_PATH "pvm.o"
#define PVM_IMG_SLOT 0
#define PVM_IMG_GPA 0x0
#define PVM_PAGE_SIZE 0x1000UL

#define PVM_STACK_SLOT PVM_IMG_SLOT+1
#define PVM_STACK_GPA 0xcd000000
#define PVM_STACK_SIZE 0x4000

#define GIC_DIST_BASE  0x08000000ULL
#define GIC_REDIST_BASE 0x080A0000ULL

struct vmm_device devices[] = {
  {
    .paddr = 0x67000000,
    .size = 0x1000,
    .read = bridge_device_read,
    .write = bridge_device_write
  }
};

static void vmm_handle_mmio_exit(msgq_t* q, struct kvm_run* run) {

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
      dev->write(q, gpa, *(unsigned long *) data);
    else if (dev->read)
      dev->read(q, gpa);

    break;
  }
}

static int vmm_handle_vm_exit(msgq_t* q, struct kvm_run* run) {

  switch (run->exit_reason) {

    case KVM_EXIT_MMIO:
      vmm_handle_mmio_exit(q, run);
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

  fd = open(PVM_IMG_PATH, O_RDONLY | O_CLOEXEC);

  if (fd < 0) {

    ret = -errno;
    goto out;
  }

  size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  // rounded to the nearest page
  map_size = (size + PVM_PAGE_SIZE - 1) & ~(PVM_PAGE_SIZE - 1);
  mem = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (mem == MAP_FAILED) {

    ret = -errno;
    goto close_fd;
  }

  stack = mmap(0, PVM_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (stack == MAP_FAILED) {

    ret = -errno;
    goto unmap_mem;
  }

  // write the file into the mmapped region
  while (done < size) {

    ret = read(fd, mem + done, size - done);

    if (ret < 0) {

      ret = -errno;
      goto unmap;
    }

    if (ret == 0) {

      ret = -EIO;
      goto unmap;
    }

    done += ret;
  }

  ret = kvm_set_mem(vm_fd, PVM_IMG_SLOT, PVM_IMG_GPA, map_size, mem);

  if (ret < 0) {

    ret = -errno;
    goto unmap;
  }

  ret = kvm_set_mem(vm_fd, PVM_STACK_SLOT, PVM_STACK_GPA, PVM_STACK_SIZE, stack);

  if (ret < 0) {

    ret = -errno;
    goto unmap;
  }

  goto close_fd;

unmap:
  munmap(stack, PVM_STACK_SIZE);
unmap_mem:
  munmap(mem, map_size);
close_fd:
  close(fd);
out:
  return ret;
}

static int pvm_create_vgic(int vm_fd) {

    struct kvm_create_device vgic = {
        .type = KVM_DEV_TYPE_ARM_VGIC_V3,
        .flags = 0,
    };

    if (ioctl(vm_fd, KVM_CREATE_DEVICE, &vgic) < 0) {
        print_err("KVM_CREATE_DEVICE", 0);
        return -1;
    }

    return vgic.fd;
}

static int set_vgic_addr(int vgic_fd, unsigned long addr, unsigned int type) {

    struct kvm_device_attr attr = {
        .group = KVM_DEV_ARM_VGIC_GRP_ADDR,
        .attr  = type,
        .addr  = (unsigned long) &addr,
    };

    return ioctl(vgic_fd, KVM_SET_DEVICE_ATTR, &attr);
}

static int pvm_init_vgic(int vm_fd) {

  int vgic = pvm_create_vgic(vm_fd);

  if (vgic < 0)
    return -1;

  set_vgic_addr(vgic, GIC_DIST_BASE, KVM_VGIC_V3_ADDR_TYPE_DIST);
  set_vgic_addr(vgic, GIC_REDIST_BASE, KVM_VGIC_V3_ADDR_TYPE_REDIST);

  struct kvm_device_attr attr = {
    .group = KVM_DEV_ARM_VGIC_GRP_CTRL,
    .attr  = KVM_DEV_ARM_VGIC_CTRL_INIT,
  };

  if (ioctl(vgic, KVM_SET_DEVICE_ATTR, &attr) < 0) {
      print_err("VGIC initialization", 0);
      return -1;
  }

  return 0;
}

static int pvm_init_perif(int vm_fd) {

  int ret = pvm_init_vgic(vm_fd);

  return ret;
}

static int vmm_init_pvm_vcpu(int vm_fd) {

  int vcpu_fd = (int) kvm_create_vcpu(vm_fd, (unsigned long) 0);
  kvm_init_vcpu(vm_fd, vcpu_fd);

  kvm_set_reg(vcpu_fd, KVM_REG_PC, PVM_IMG_GPA);
  kvm_set_reg(vcpu_fd, KVM_CORE_REG(sp_el1), PVM_STACK_GPA + PVM_STACK_SIZE);

  return vcpu_fd;
}

static struct kvm_run* vmm_init_pvm_run_obj(int kvm_fd, int vcpu_fd) {

  long mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  return (struct kvm_run *) mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0);
}

int vmm_init_pvm(int kvm_fd, int vm_fd) {

  long ret = pvm_init_mem(kvm_fd, vm_fd);

  if (ret)
    print_err("load " PVM_IMG_PATH, ret);

  return (int)ret;
}

void vmm_run_pvm(msgq_t* q, int kvm_fd, int vm_fd) {

  int vcpu_fd = vmm_init_pvm_vcpu(vm_fd);
  struct kvm_run* run = vmm_init_pvm_run_obj(kvm_fd, vcpu_fd);

  if (pvm_init_perif(vm_fd))
    return;

  print("running...\n");

  while(1) {

    kvm_run_vcpu(vcpu_fd);
    if (vmm_handle_vm_exit(q, run))
      return;
  }
}

