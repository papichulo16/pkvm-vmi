#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/kvm_pkvm_module.h>

#include "pkvmi.h"
#include "pkvmi_args.h"

int __kvm_nvhe_pkvmi_init(const struct pkvm_module_ops *ops);
void __kvm_nvhe_pkvmi_hvc(struct kvm_cpu_context *ctx);

char* g_msg;
static int pkvmi_hvc;
static struct page *shared_page;

unsigned long pkvm_driver_init(void) {

  unsigned long token;
  unsigned long ret;
  int err;

  err = pkvm_load_el2_module(__kvm_nvhe_pkvmi_init, &token);
  ret = PKVMI_ERR(PKVMI_ERR_EL2_LOAD, err);

  if (err)
    goto end;

  err = pkvm_register_el2_mod_call(__kvm_nvhe_pkvmi_hvc, token);
  ret = PKVMI_ERR(PKVMI_ERR_HVC_REGISTER, err);

  if (err < 0)
    goto end;

  pkvmi_hvc = err;
  ret = PKVMI_OK;

end:
  return ret;
}

static unsigned long pkvmi_init_hvc(void) {

  unsigned long ret = PKVMI_OK;

  if (shared_page)
    goto end;

  shared_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
  ret = PKVMI_ERR(PKVMI_ERR_SHARED_ALLOC, -ENOMEM);

  if (!shared_page)
    goto end;

  ret = pkvm_el2_mod_call(pkvmi_hvc, PKVMI_INIT, kimage_voffset, pkvm_vm_table_pa, page_to_pfn(shared_page));

  if (ret) {
    __free_page(shared_page);
    shared_page = NULL;
  }

end:
  return ret;
}

static int pkvmi_reg_dump_hvc(void) {

  if (!shared_page)
    return -ENODEV;

  return pkvm_el2_mod_call(pkvmi_hvc, PKVMI_REG_DUMP);
}

static int pkvmi_reg_dump_el1_hvc(void) {

  if (!shared_page)
    return -ENODEV;

  return pkvm_el2_mod_call(pkvmi_hvc, PKVMI_REG_DUMP_EL1);
}

static long pkvmi_page_to_user(unsigned long arg) {

  return copy_to_user((void __user *)arg, page_address(shared_page), IOCTL_MSG_LEN) ? -EFAULT : 0;
}

static long pkvmi_read_ioctl(unsigned long arg, unsigned long hvc_id) {

  long ret = -EFAULT;
  unsigned long addr;
  long r;

  if (copy_from_user(&addr, (void __user *)arg, sizeof(addr)))
    goto end;

  if (hvc_id == PKVMI_READ_PA_EL1 && !pfn_valid(PHYS_PFN(addr)))
    goto end;

  ret = -ENODEV;

  if (!shared_page)
    goto end;

  r = (int) pkvm_el2_mod_call(pkvmi_hvc, hvc_id, addr);
  ret = r < 0 ? r : -EIO;

  if (r)
    goto end;

  ret = pkvmi_page_to_user(arg);

end:
  return ret;
}

long pkvmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	long ret = -ENOTTY;
  unsigned long r;

  switch (cmd) {

    case HYPM_INIT:

      r = pkvmi_init_hvc();
      pkvmi_log_err(r);

      if (!r)
        g_msg = "lets goo jit";
      else
        g_msg = "bruh";

      ret = copy_to_user((void __user *)arg, g_msg, strlen(g_msg) + 1) ? -EFAULT : 0;

      break;

    case HYPM_REG_DUMP:

      r = pkvmi_reg_dump_hvc();

      ret = r ? -EIO : pkvmi_page_to_user(arg);

      break;

    case HYPM_READ_PA:

      ret = pkvmi_read_ioctl(arg, PKVMI_READ_PA);
      break;

    case HYPM_READ_VA:

      ret = pkvmi_read_ioctl(arg, PKVMI_READ_VA);
      break;

    case HYPM_REG_DUMP_EL1:

      r = pkvmi_reg_dump_el1_hvc();

      ret = r ? -EIO : pkvmi_page_to_user(arg);

      break;

    case HYPM_READ_PA_EL1:

      ret = pkvmi_read_ioctl(arg, PKVMI_READ_PA_EL1);
      break;

    case HYPM_READ_VA_EL1:

      ret = pkvmi_read_ioctl(arg, PKVMI_READ_VA_EL1);
      break;
  }

	return ret;
}

