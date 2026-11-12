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
endif
