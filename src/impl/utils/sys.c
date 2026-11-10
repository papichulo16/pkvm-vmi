#include "sys.h"

long sys_call(long nr, long a0, long a1, long a2, long a3, long a4, long a5) {

	register long x8 __asm__("x8") = nr;
	register long x0 __asm__("x0") = a0;
	register long x1 __asm__("x1") = a1;
	register long x2 __asm__("x2") = a2;
	register long x3 __asm__("x3") = a3;
	register long x4 __asm__("x4") = a4;
	register long x5 __asm__("x5") = a5;

	__asm__ volatile("svc #0"
			 : "+r"(x0)
			 : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
			 : "memory");
	return x0;
}

long sys_open(const char *path, int flags) {

	return sys_call(__NR_openat, AT_FDCWD, (long)path, flags | O_CLOEXEC, 0, 0, 0);
}

long sys_close(int fd) {

	return sys_call(__NR_close, fd, 0, 0, 0, 0, 0);
}

long sys_read(int fd, void *buf, unsigned long len) {

	return sys_call(__NR_read, fd, (long)buf, (long)len, 0, 0, 0);
}

long sys_write(int fd, const void *buf, unsigned long len) {

	return sys_call(__NR_write, fd, (long)buf, (long)len, 0, 0, 0);
}

long sys_ioctl(int fd, unsigned long req, unsigned long arg) {

	return sys_call(__NR_ioctl, fd, (long)req, (long)arg, 0, 0, 0);
}

void *sys_mmap(void *addr, unsigned long len, int prot, int flags, int fd, long off) {

	return (void *)sys_call(__NR_mmap, (long)addr, (long)len, prot, flags, fd, off);
}

long sys_munmap(void *addr, unsigned long len) {

	return sys_call(__NR_munmap, (long)addr, (long)len, 0, 0, 0, 0);
}

void __attribute__((noreturn)) sys_exit(int code) {

	sys_call(__NR_exit_group, code, 0, 0, 0, 0, 0);
	for (;;)
		;
}

void *memset(void *dst, int c, unsigned long len) {

	unsigned char *d = dst;

	while (len--)
		*d++ = (unsigned char)c;

	return dst;
}

void *memcpy(void *dst, const void *src, unsigned long len) {

	unsigned char *d = dst;
	const unsigned char *s = src;

	while (len--)
		*d++ = *s++;

	return dst;
}
