#include "cgra.h"

#include "common.h"
#include "config.h"
#include "config2.h"
#include "cpuinfo.h"
#include "devices.h"
#include "pocl_builtin_kernels.h"
#include "pocl_local_size.h"
#include "pocl_util.h"
#include "topology/pocl_topology.h"
#include "utlist.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <utlist.h>
#include <stdio.h>

#include "pocl_cache.h"
#include "pocl_file_util.h"
#include "pocl_mem_management.h"
#include "pocl_timing.h"
#include "pocl_workgroup_func.h"

typedef struct
{
  /* List of commands ready to be executed */
  _cl_command_node *ready_list;
  /* List of commands not yet ready to be executed */
  _cl_command_node *command_list;
  /* Lock for command list related operations */
  pocl_lock_t cq_lock;

  /* printf buffer */
  void *printf_buffer;

  cl_bool available;
} pocl_cgra_data_t;

void
pocl_cgra_init_device_ops(struct pocl_device_ops *ops)
{
  ops->device_name = "cgra";
  ops->probe = pocl_cgra_probe;
  ops->init = pocl_cgra_init;
  ops->build_hash = pocl_cgra_build_hash;

  // Memory
  ops->alloc_mem_obj = pocl_cgra_alloc_mem_obj;
  ops->free = pocl_cgra_free;
  ops->map_mem = pocl_cgra_map_mem;
  ops->write = pocl_cgra_write;
  ops->read = pocl_cgra_read;
}

unsigned int
pocl_cgra_probe (struct pocl_device_ops *ops)
{
  int env_count = pocl_device_get_env_count(ops->device_name);
  if (env_count < 0) {
    return 0;
  }
  else {
    return env_count;
  }
}

cl_int
pocl_cgra_init (unsigned j, cl_device_id device, const char* parameters)
{
  cl_int ret = CL_SUCCESS;

  pocl_init_default_device_infos(device, "");

  device->type = CL_DEVICE_TYPE_ACCELERATOR;
  device->long_name = (char *)"mm-reconfigurable-accelerator-device";
  device->short_name = "cgra";
  device->vendor = "CGRA PoCL";
  device->version = "OpenCL 3.0 PoCL";
  device->extensions = "";
  device->profile = "FULL_PROFILE";
  /*
  device->max_mem_alloc_size = 100 * 1024 * 1024;
  device->mem_base_addr_align = 8;
  device->max_constant_buffer_size = 32768;
  device->local_mem_size = 16384;
  device->address_bits = 32;
  device->max_compute_units = 1;
  device->image_support = CL_FALSE;
  device->profiling_timer_resolution = 1000;
  */
  device->max_work_item_dimensions = 3;
  /*
  device->max_work_group_size = device->max_work_item_sizes[0] =
      device->max_work_item_sizes[1] = device->max_work_item_sizes[2] = 1024;
  device->max_work_item_sizes[0] = device->max_work_item_sizes[1] =
      device->max_work_item_sizes[2] = device->max_work_group_size = 64;
  */

  pocl_cgra_data_t *d;
  d = (pocl_cgra_data_t *)calloc (1, sizeof (pocl_cgra_data_t));
  if (d == NULL)
    return CL_OUT_OF_HOST_MEMORY;

  d->available = CL_TRUE;
  device->available = &(d->available);
  device->compiler_available = CL_FALSE;
  device->linker_available = CL_FALSE;
  device->data = (void *)d;

  printf("CGRA::init\n");
  return ret;
}

cl_int pocl_cgra_alloc_mem_obj(cl_device_id device, cl_mem mem_obj, void *host_ptr)
{
  cl_int ret = CL_SUCCESS;
  printf("CGRA::alloc_mem_obj\n");
  return ret;
}

void pocl_cgra_free(cl_device_id device, cl_mem mem)
{
  printf("CGRA::free\n");
  pocl_mem_identifier *p = &mem->device_ptrs[device->global_mem_id];
  p->mem_ptr = NULL;
  p->version = 0;
}

cl_int pocl_cgra_map_mem(void *data, pocl_mem_identifier *src_mem_id, cl_mem src_buf, mem_mapping_t *map)
{
  printf("CGRA::map_mem\n");
  return CL_SUCCESS;
}

char *
pocl_cgra_build_hash (cl_device_id device)
{
  char *res = calloc(1000, sizeof(char));
  snprintf (res, 1000, "cgra-runtime");
  return res;
}

void
pocl_cgra_write (void *data,
                 const void *__restrict__  src_host_ptr,
                 pocl_mem_identifier * dst_mem_id,
                 cl_mem dst_buf,
                 size_t offset, size_t size)
{

}

void
pocl_cgra_read (void *data,
                void *__restrict__  dst_host_ptr,
                pocl_mem_identifier * src_mem_id,
                cl_mem src_buf,
                size_t offset,
                size_t size)
{

}