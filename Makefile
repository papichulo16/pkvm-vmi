KDIR ?= $(abspath $(CURDIR)/ignore/out-qemu-6.1)
LLVM ?= 1

# local  : pkvmi.ko + `userspace`, the CLI that talks to /dev/pkvmi directly
# bridge : pkvmi.ko + `insp-vmm`, a host VMM that runs the inspection pVM
PKVMI_MODE ?= local

ifeq ($(filter $(PKVMI_MODE),local bridge),)
$(error PKVMI_MODE must be local or bridge, got '$(PKVMI_MODE)')
endif

# built by scripts/fetch-musl.sh
MUSL ?= $(abspath $(CURDIR)/ignore/musl-aarch64)

# crt files are listed by hand because the host clang has no aarch64 libgcc/compiler-rt
g_usr_cflags := --target=aarch64-linux-musl -O2 -static -nostdlib -nostdinc \
	-fuse-ld=lld -isystem $(MUSL)/include -isystem $(KDIR)/usr/include
g_usr_crt_begin := $(MUSL)/lib/crt1.o $(MUSL)/lib/crti.o
g_usr_crt_end := $(MUSL)/lib/libc.a $(MUSL)/lib/crtn.o

# the pVM guest has no OS under it, so it stays freestanding
g_pvm_cflags := --target=aarch64-linux-gnu -O2 -static -nostdlib -ffreestanding \
	-fno-builtin -fno-stack-protector -fuse-ld=lld -isystem $(KDIR)/usr/include

g_utils_src := $(sort $(wildcard $(CURDIR)/src/impl/utils/*.c))
g_local_src := $(sort $(wildcard $(CURDIR)/src/impl/vmi/usr/*.c))
g_pvm_src := $(CURDIR)/src/impl/insp-vm/pvm.c
g_bridge_src := $(filter-out $(g_pvm_src),$(sort $(wildcard $(CURDIR)/src/impl/insp-vm/*.c)))

g_usr_bin_local := userspace
g_usr_bin_bridge := insp-vmm pvm.o

all: module $(g_usr_bin_$(PKVMI_MODE))

local:
	$(MAKE) PKVMI_MODE=local

bridge:
	$(MAKE) PKVMI_MODE=bridge

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR)/src ARCH=arm64 LLVM=$(LLVM) PKVMI_MODE=$(PKVMI_MODE) modules

userspace: $(g_local_src) $(g_utils_src) $(wildcard $(CURDIR)/src/intf/utils/*.h)
	clang $(g_usr_cflags) -I src/intf -I src/intf/utils -o $@ $(g_usr_crt_begin) $(g_local_src) $(g_utils_src) $(g_usr_crt_end)

insp-vmm: $(g_bridge_src) $(g_utils_src) $(wildcard $(CURDIR)/src/intf/insp-vm/*.h $(CURDIR)/src/intf/utils/*.h)
	clang $(g_usr_cflags) -I src/intf -I src/intf/insp-vm -I src/intf/utils -o $@ $(g_usr_crt_begin) $(g_bridge_src) $(g_utils_src) $(g_usr_crt_end)

# pvm.o is a flat binary, not an ELF: insp-vmm copies it byte for byte to guest
# address 0, so the first instruction has to be the first byte of the file
pvm.o: $(g_pvm_src)
	clang $(g_pvm_cflags) -Wl,--image-base=0 -Wl,-Ttext=0 -Wl,-e,0 -Wl,--build-id=none -o $@.elf $<
	llvm-objcopy -O binary -j .text $@.elf $@
	rm -f $@.elf

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR)/src ARCH=arm64 LLVM=$(LLVM) clean
	rm -f userspace insp-vmm pvm.o

.PHONY: all local bridge module clean
