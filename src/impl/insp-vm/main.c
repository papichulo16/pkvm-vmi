#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "kvm.h"
#include "vmm.h"
#include "print.h"
#include "user_msgq.h"

#define MSGQ_SIZE 8
#define USER_INPUT_SIZE 0x100

static void* vmm_init(void* arg) {

	long kvm_fd;
	long vm_fd;
	int ret = 1;
  msgq_t* q = (msgq_t *) arg;

	kvm_fd = kvm_open();

	if (kvm_fd < 0) {

		print_err("open /dev/kvm", -errno);
		goto out;
	}

	vm_fd = kvm_create_pvm((int)kvm_fd, 0);

	if (vm_fd < 0) {

		print_err("create protected vm", -errno);
		goto close_kvm;
	}

	printf("protected vm fd %ld\n", vm_fd);

  if (vmm_init_pvm(kvm_fd, vm_fd)) {

		print_err("error in vmm init", 67);
    goto close_kvm;
  }

  vmm_run_pvm(q, kvm_fd, vm_fd);

	ret = 0;
	close((int)vm_fd);

close_kvm:
	close((int)kvm_fd);
out:
	return NULL;
}

static void* user_thread(void* arg) {

  char input[USER_INPUT_SIZE];
  msgq_t* q = (msgq_t *) arg;

  while (1) {

    fgets(input, sizeof(input), stdin);
    msgq_enq(q, input);
  }

  return NULL;
}

int main(int argc, char **argv) {

  pthread_t vmm_tid, user_tid;
  msgq_t* q;

  (void) argc;
  (void) argv;

  q = msgq_init(MSGQ_SIZE);

  pthread_create(&vmm_tid, NULL, vmm_init, q);
  pthread_create(&user_tid, NULL, user_thread, q);

  pthread_join(vmm_tid, NULL);

  pthread_kill(user_tid, SIGKILL);
  pthread_join(user_tid, NULL);

  msgq_free(q);

  return 0;
}

