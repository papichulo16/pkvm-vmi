#ifndef INSP_SYS_H
#define INSP_SYS_H

#include <asm/unistd.h>
#include <linux/fcntl.h>
#include <linux/errno.h>
#include <linux/mman.h>

/* Raw aarch64 Linux syscalls: the VMM is built -nostdlib, so these replace libc. */

long sys_call(long nr, long a0, long a1, long a2, long a3, long a4, long a5);

long sys_open(const char *path, int flags);
long sys_close(int fd);
long sys_read(int fd, void *buf, unsigned long len);
long sys_write(int fd, const void *buf, unsigned long len);
long sys_ioctl(int fd, unsigned long req, unsigned long arg);
void *sys_mmap(void *addr, unsigned long len, int prot, int flags, int fd, long off);
long sys_munmap(void *addr, unsigned long len);
void __attribute__((noreturn)) sys_exit(int code);

/* the kernel returns -errno in x0, so anything in [-4095, -1] is an error */
static inline int sys_is_err(long ret) {

	return (unsigned long)ret >= (unsigned long)-4095;
}

static inline int sys_mmap_failed(void *p) {

	return sys_is_err((long)p);
}

/* clang may emit calls to these for struct copies/zeroing even with -fno-builtin */
void *memset(void *dst, int c, unsigned long len);
void *memcpy(void *dst, const void *src, unsigned long len);

#endif
