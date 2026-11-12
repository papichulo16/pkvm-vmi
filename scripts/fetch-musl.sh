#!/usr/bin/env bash
# Download musl and build a static aarch64 sysroot with clang into
# ignore/musl-aarch64 (include/ + lib/crt*.o + lib/libc.a) for src/Makefile.
set -euo pipefail

g_work=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/ignore
g_ver=1.2.5
g_sha=a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4
g_file=musl-$g_ver.tar.gz
g_out=$g_work/musl-aarch64

die() {
	echo "fetch-musl: $*" >&2
	exit 1
}

main() {
	local build=$g_work/musl-$g_ver

	[[ ! -e $g_out ]] || die "$g_out exists, delete it to rebuild"

	mkdir -p "$g_work"
	curl -fsS -o "$g_work/$g_file" "https://musl.libc.org/releases/$g_file"
	echo "$g_sha  $g_work/$g_file" | sha256sum -c - || die "checksum mismatch"

	rm -rf "$build"
	tar -xzf "$g_work/$g_file" -C "$g_work"

	(
		cd "$build"
		./configure --target=aarch64-linux-musl --prefix="$g_out" --disable-shared \
			CC="clang --target=aarch64-linux-musl" AR=llvm-ar RANLIB=llvm-ranlib
		make -j"$(nproc)"
		make install
	)

	rm -rf "$build" "$g_work/$g_file"
	echo "fetch-musl: musl $g_ver -> $g_out"
}

main "$@"
