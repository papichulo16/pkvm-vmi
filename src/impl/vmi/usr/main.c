#include "pkvmi.h"
#include "pkvmi_regs.h"
#include "print.h"
#include "sys.h"

#define PAGE_SIZE 0x1000

#define LINE_LEN 256

static void print_byte_hex(unsigned char b) {

	char buf[2];
	int hi = b >> 4;
	int lo = b & 0xf;

	buf[0] = hi < 10 ? '0' + hi : 'a' + hi - 10;
	buf[1] = lo < 10 ? '0' + lo : 'a' + lo - 10;

	sys_write(1, buf, sizeof(buf));
}

static const char *skip_spaces(const char *s) {

	while (*s == ' ' || *s == '\t')
		s++;

	return s;
}

static int tok_eq(const char *line, const char *cmd, const char **rest) {

	const char *l = skip_spaces(line);

	while (*cmd) {
		if (*l != *cmd)
			return 0;
		l++;
		cmd++;
	}

	if (*l != 0 && *l != ' ' && *l != '\t')
		return 0;

	*rest = skip_spaces(l);

	return 1;
}

static unsigned long parse_hex(const char *s, int *ok) {

	unsigned long v = 0;
	int any = 0;

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;

	while (*s) {
		char c = *s;
		int d;

		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'f')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			d = c - 'A' + 10;
		else
			break;

		v = (v << 4) | d;
		any = 1;
		s++;
	}

	*ok = any;

	return v;
}

static long read_line(char *buf, long max) {

	long len = 0;

	while (len < max - 1) {
		char c;
		long r = sys_read(0, &c, 1);

		if (r <= 0)
			return len == 0 ? -1 : len;

		if (c == '\n')
			break;

		buf[len++] = c;
	}

	buf[len] = 0;

	return len;
}

static void cmd_regdump(int fd) {

	char page[IOCTL_MSG_LEN];
	struct hyp_reg_dump *r = (struct hyp_reg_dump *)page;

	if (sys_ioctl(fd, HYPM_REG_DUMP, (unsigned long)page) != 0) {
		print("regdump failed\n");
		return;
	}

	print("hcr_el2   "); print_hex(r->hcr_el2);   print("\n");
	print("sctlr_el2 "); print_hex(r->sctlr_el2); print("\n");
	print("tcr_el2   "); print_hex(r->tcr_el2);   print("\n");
	print("ttbr0_el2 "); print_hex(r->ttbr0_el2); print("\n");
	print("mair_el2  "); print_hex(r->mair_el2);  print("\n");
	print("vbar_el2  "); print_hex(r->vbar_el2);  print("\n");
	print("esr_el2   "); print_hex(r->esr_el2);   print("\n");
	print("elr_el2   "); print_hex(r->elr_el2);   print("\n");
	print("spsr_el2  "); print_hex(r->spsr_el2);  print("\n");
	print("tpidr_el2 "); print_hex(r->tpidr_el2); print("\n");
	print("vttbr_el2 "); print_hex(r->vttbr_el2); print("\n");
	print("vtcr_el2  "); print_hex(r->vtcr_el2);  print("\n");
}

static void cmd_regdump_el1(int fd) {

	char page[IOCTL_MSG_LEN];
	struct el1_reg_dump *r = (struct el1_reg_dump *)page;

	if (sys_ioctl(fd, HYPM_REG_DUMP_EL1, (unsigned long)page) != 0) {
		print("regdump_el1 failed\n");
		return;
	}

	print("sctlr_el1 "); print_hex(r->sctlr_el1); print("\n");
	print("tcr_el1   "); print_hex(r->tcr_el1);   print("\n");
	print("ttbr0_el1 "); print_hex(r->ttbr0_el1); print("\n");
	print("ttbr1_el1 "); print_hex(r->ttbr1_el1); print("\n");
	print("mair_el1  "); print_hex(r->mair_el1);  print("\n");
	print("vbar_el1  "); print_hex(r->vbar_el1);  print("\n");
	print("esr_el1   "); print_hex(r->esr_el1);   print("\n");
	print("elr_el1   "); print_hex(r->elr_el1);   print("\n");
	print("spsr_el1  "); print_hex(r->spsr_el1);  print("\n");
	print("sp_el1    "); print_hex(r->sp_el1);    print("\n");
	print("tpidr_el1 "); print_hex(r->tpidr_el1); print("\n");
	print("par_el1   "); print_hex(r->par_el1);   print("\n");
}

static void print_read_err(const char *name, long err) {

	print(name);

	if (err == -EFAULT)
		print(": address not mapped\n");
	else if (err == -ENODEV)
		print(": not initialized\n");
	else {
		print(": failed, errno ");
		print_hex(-err);
		print("\n");
	}
}

static void cmd_read(int fd, const char *arg, unsigned long ioctl_nr, const char *name) {

	char page[IOCTL_MSG_LEN];
	unsigned long addr;
	long err;
	int ok;
	int row;
	int col;

	addr = parse_hex(arg, &ok) & ~(PAGE_SIZE - 1);

	if (!ok) {
		print("usage: ");
		print(name);
		print(" <hex-addr>\n");
		return;
	}

	*(unsigned long *)page = addr;
	err = sys_ioctl(fd, ioctl_nr, (unsigned long)page);

	if (err) {
		print_read_err(name, err);
		return;
	}

	for (row = 0; row < (int) (IOCTL_MSG_LEN / 0x10); row++) {
		print_hex(addr + row * 16);
		print("  ");

		for (col = 0; col < 2; col++) {
			print_hex(((unsigned long *) page)[row * 2 + col]);
			print(" ");
		}

		print("\n");
	}
}

static void cmd_help(void) {

	print("commands:\n");
	print("  regdump             dump EL2 system registers\n");
	print("  readpa <hex-pa>     read a physical page through the EL2 linear map\n");
	print("  readva <hex-va>     read an EL2 virtual page\n");
	print("  regdump_el1         dump EL1 system registers\n");
	print("  readpa_el1 <hex-pa> read a physical page through a temp EL2 map\n");
	print("  readva_el1 <hex-va> read an EL1 virtual page\n");
	print("  help                show this\n");
	print("  exit                quit\n");
}

void __attribute__((noreturn)) _start(void) {

	char line[LINE_LEN];
	char msg[IOCTL_MSG_LEN];
	long fd = sys_open("/dev/pkvmi", O_RDWR);

	if (fd < 0) {
		print("cannot open /dev/pkvmi\n");
		sys_exit(1);
	}

	if (sys_ioctl(fd, HYPM_INIT, (unsigned long)msg) != 0)
		print("init failed\n");
	else
		print("pkvmi init: ");

	print(msg);
	print("\n");

	for (;;) {
		const char *rest;
		long len;

		print("pkvmi> ");

		len = read_line(line, LINE_LEN);

		if (len < 0)
			break;

		if (tok_eq(line, "regdump_el1", &rest))
			cmd_regdump_el1((int)fd);
		else if (tok_eq(line, "readpa_el1", &rest))
			cmd_read((int)fd, rest, HYPM_READ_PA_EL1, "readpa_el1");
		else if (tok_eq(line, "readva_el1", &rest))
			cmd_read((int)fd, rest, HYPM_READ_VA_EL1, "readva_el1");
		else if (tok_eq(line, "regdump", &rest))
			cmd_regdump((int)fd);
		else if (tok_eq(line, "readpa", &rest))
			cmd_read((int)fd, rest, HYPM_READ_PA, "readpa");
		else if (tok_eq(line, "readva", &rest))
			cmd_read((int)fd, rest, HYPM_READ_VA, "readva");
		else if (tok_eq(line, "help", &rest))
			cmd_help();
		else if (tok_eq(line, "exit", &rest) || tok_eq(line, "quit", &rest))
			break;
		else if (*skip_spaces(line))
			print("unknown command (try 'help')\n");
	}

	print("bye\n");
	sys_exit(0);
}

