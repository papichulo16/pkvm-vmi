#include "print.h"
#include "sys.h"

static unsigned long str_len(const char *s) {

	unsigned long len = 0;

	while (s[len])
		len++;

	return len;
}

void print(const char *str) {

	sys_write(1, str, str_len(str));
}

void print_hex(unsigned long v) {

	char buf[18];
	int i;

	buf[0] = '0';
	buf[1] = 'x';

	for (i = 0; i < 16; i++) {
		int nib = (v >> ((15 - i) * 4)) & 0xf;

		buf[2 + i] = nib < 10 ? '0' + nib : 'a' + nib - 10;
	}

	sys_write(1, buf, sizeof(buf));
}

void print_err(const char *what, long err) {

	print(what);
	print(" failed, errno ");
	print_hex(-err);
	print("\n");
}
