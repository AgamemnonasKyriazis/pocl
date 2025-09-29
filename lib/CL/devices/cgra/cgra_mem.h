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

#include "pocl_cl.h"
#include "pocl_util.h"

#define DEVICE_MEM_BASE_ADDR 0x2000u

int cgra_write_to_device (
    void *data, const void *__restrict__ host_ptr,
    pocl_mem_identifier *dst_mem_id, cl_mem dst_buf,
    size_t offset, size_t size);

int cgra_read_from_device (
    void *data, void *__restrict__ host_ptr,
    pocl_mem_identifier *src_mem_id, cl_mem src_buf,
    size_t offset, size_t size);

#endif //POCL_CGRA_MEM_H