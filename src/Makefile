ifneq ($(KERNELRELEASE),)
# Kleaf passes M relative to the kernel source, QEMU builds pass it absolute
g_src_dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
g_inc := $(abspath $(g_src_dir)/intf)
subdir-ccflags-y += -I$(srctree)/arch/arm64/kvm/hyp/include/ -I$(g_inc)
ccflags-y += -I$(g_inc)

# PKVMI_MODE=bridge lets the EL1/EL2 code compile in what the inspection VM needs
ifeq ($(PKVMI_MODE),bridge)
subdir-ccflags-y += -DPKVMI_MODE_BRIDGE
ccflags-y += -DPKVMI_MODE_BRIDGE
endif

g_kern_src := $(sort $(wildcard $(g_src_dir)/impl/vmi/kern/*.c))
g_kern_obj := $(patsubst $(g_src_dir)/%.c,%.o,$(g_kern_src))

obj-m := pkvmi.o
pkvmi-y := $(g_kern_obj) impl/vmi/hyp/kvm_nvhe.o

$(obj)/impl/vmi/hyp/kvm_nvhe.o: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/impl/vmi/hyp $(obj)/impl/vmi/hyp/kvm_nvhe.o

clean-files := impl/vmi/hyp/hyp.lds impl/vmi/hyp/hyp-reloc.S
else ifneq ($(KERNEL_SRC),)

# Kleaf (scripts/pixel/build-device.sh) runs
#   make -C <this dir> KERNEL_SRC=<kernel> M=<rel dir> O=<out> [modules_install]
# with the toolchain on the command line, like Google's own modules. Its first
# call names no goal, so `modules` must stay the first target.
modules modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $@
else
KDIR ?= $(abspath $(CURDIR)/../ignore/out-qemu-6.1)
LLVM ?= 1

# local  : pkvmi.ko + `userspace`, the CLI that talks to /dev/pkvmi directly
# bridge : pkvmi.ko + `insp-vmm`, a host VMM that runs the inspection pVM
PKVMI_MODE ?= local

ifeq ($(filter $(PKVMI_MODE),local bridge),)
$(error PKVMI_MODE must be local or bridge, got '$(PKVMI_MODE)')
endif

g_usr_cflags := --target=aarch64-linux-gnu -O2 -static -nostdlib -ffreestanding \
	-fno-builtin -fno-stack-protector -fuse-ld=lld -isystem $(KDIR)/usr/include

g_utils_src := $(sort $(wildcard $(CURDIR)/impl/utils/*.c))
g_local_src := $(sort $(wildcard $(CURDIR)/impl/vmi/usr/*.c))
g_pvm_src := $(CURDIR)/impl/insp-vm/pvm.c
g_bridge_src := $(filter-out $(g_pvm_src),$(sort $(wildcard $(CURDIR)/impl/insp-vm/*.c)))

g_usr_bin_local := userspace
g_usr_bin_bridge := insp-vmm pvm.o

all: module $(g_usr_bin_$(PKVMI_MODE))

local:
	$(MAKE) PKVMI_MODE=local

bridge:
	$(MAKE) PKVMI_MODE=bridge

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 LLVM=$(LLVM) PKVMI_MODE=$(PKVMI_MODE) modules

userspace: $(g_local_src) $(g_utils_src) $(wildcard $(CURDIR)/intf/utils/*.h)
	clang $(g_usr_cflags) -I intf -I intf/utils -o $@ $(g_local_src) $(g_utils_src)

insp-vmm: $(g_bridge_src) $(g_utils_src) $(wildcard $(CURDIR)/intf/insp-vm/*.h $(CURDIR)/intf/utils/*.h)
	clang $(g_usr_cflags) -I intf -I intf/insp-vm -I intf/utils -o $@ $(g_bridge_src) $(g_utils_src)

# pvm.o is a flat binary, not an ELF: insp-vmm copies it byte for byte to guest
# address 0, so the first instruction has to be the first byte of the file
pvm.o: $(g_pvm_src)
	clang $(g_usr_cflags) -Wl,--image-base=0 -Wl,-Ttext=0 -Wl,-e,0 -Wl,--build-id=none -o $@.elf $<
	llvm-objcopy -O binary -j .text $@.elf $@
	rm -f $@.elf

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 LLVM=$(LLVM) clean
	rm -f userspace insp-vmm pvm.o

.PHONY: all local bridge module clean
endif

