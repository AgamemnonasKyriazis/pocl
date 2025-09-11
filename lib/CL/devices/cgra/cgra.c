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

typedef struct _pocl_cgra_usm_allocation_t
{
  void *ptr;
  size_t size;
  cl_mem_alloc_flags_intel flags;
  unsigned alloc_type;

  struct _pocl_cgra_usm_allocation_t *next, *prev;
} pocl_cgra_usm_allocation_t;

void
pocl_cgra_init_device_ops(struct pocl_device_ops *ops)
{
  ops->device_name = "cgra";
  ops->probe = pocl_cgra_probe;
  ops->init = pocl_cgra_init;

  // Memory
  ops->alloc_mem_obj = pocl_cgra_alloc_mem_obj;
  ops->free = pocl_cgra_free;
  ops->map_mem = pocl_cgra_map_mem;
  printf("CGRA::init_device_ops\n");
}

unsigned int
pocl_cgra_probe (struct pocl_device_ops *ops)
{
  int env_count = pocl_device_get_env_count(ops->device_name);
  printf("CGRA::ENV_COUNT::%d %s\n", env_count, ops->device_name);

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

  device->type 				= CL_DEVICE_TYPE_CUSTOM;
  device->long_name 			= (char *)"Coarse Grain Reconfigurable Device";
  device->short_name 			= "cgra";
  device->vendor 			= "pocl";
  device->version 			= "1.2";
  device->extensions 			= "";
  device->address_bits   		= 32;
  device->max_compute_units    	   	= 1;
  device->max_work_item_dimensions 	= 3;
  device->image_support        	   	= CL_FALSE;
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
