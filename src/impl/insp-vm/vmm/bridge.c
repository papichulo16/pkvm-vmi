#include "bridge.h"
#include "print.h"

void bridge_device_read(void* arg, unsigned long off) {

  print("bridge read\n");
}

void bridge_device_write(void* arg, unsigned long off, unsigned long val) {

  print("bridge write\n");
}

