#ifndef INSP_BRIDGE_H
#define INSP_BRIDGE_H

void bridge_device_read(void* arg, unsigned long off);
void bridge_device_write(void* arg, unsigned long off, unsigned long val);

#endif

