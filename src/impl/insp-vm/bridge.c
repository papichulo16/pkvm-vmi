#include "bridge.h"
#include "print.h"
#include "sys.h"

void bridge_device_read(unsigned long off) {

  print("bridge read\n");
}

void bridge_device_write(unsigned long off, unsigned long val) {

  print("bridge write\n");
}

