#ifndef POCL_CGRA_MEM_H
#define POCL_CGRA_MEM_H

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <utlist.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEVICE_MEM_BASE_ADDR 0x2000u

int cgra_mem_init ();
int cgra_mem_stream_to_device (void * host_ptr, size_t size);
int cgra_stream_from_device (void * host_ptr, size_t size);
int cgra_write_to_device (const void * __restrict__ host_ptr, size_t size);
int cgra_read_from_device (void *__restrict__ host_ptr, size_t size);

#endif //POCL_CGRA_MEM_H