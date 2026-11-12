#!/usr/bin/env bash
# Build src/ (pkvmi.ko + userspace) and pack it into the 6.1 QEMU initramfs.
# PKVMI_MODE=bridge ships /insp-vmm + /pvm.o instead of /userspace.
# Extra arguments go to dev.sh, e.g. `-r /userspace` to run it at boot.
set -euo pipefail

g_here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
g_bins=(-b userspace)

export PKVMI_MODE=${PKVMI_MODE:-local}

if [[ $PKVMI_MODE = bridge ]]; then
	g_bins=(-b insp-vmm -b pvm.o)
fi

exec "$g_here/dev.sh" -C "$g_here/.." -m src/pkvmi.ko "${g_bins[@]}" "$@"
