#include "cgra.h"
#include "cgra_mem.h"

#include "common.h"
#include "common_driver.h"
#include "common_utils.h"

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

const char * _long_name  = "Memory Mapped Reconfigurable Accelerator";
const char * _short_name = "cgra";
const char * _vendor     = "PoCL";
const char * _version    = "OpenCL 1.2 PoCL";
const char * _extensions = "\0";
const char * _profile    = "FULL_PROFILE";
const char * _hash_str   = "cgmmra-linux-gnu";

#define L_MEM_SIZE (1024*1024*8)
#define IMAGE_SUPPORT CL_FALSE

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
  ops->device_name = _short_name;
  ops->probe = pocl_cgra_probe;
  ops->init = pocl_cgra_init;
  ops->build_hash = pocl_cgra_build_hash;

  ops->setup_metadata = pocl_cgra_setup_metadata;
  ops->build_source = pocl_cgra_build_source;
  ops->link_program = NULL;
  ops->build_binary = NULL;
  ops->build_builtin = NULL;
  ops->compile_kernel = pocl_cgra_compile_kernel;
  
  ops->run = pocl_cgra_run;

  // Control
  ops->submit = pocl_cgra_submit;
  ops->join = pocl_cgra_join;
  ops->flush = pocl_cgra_flush;
  ops->broadcast = pocl_cgra_broadcast;

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
  pocl_cpu_init_common(device);
  pocl_setup_device_for_system_memory(device);

  device->type = CL_DEVICE_TYPE_ACCELERATOR;
  device->long_name = _long_name;
  device->short_name = _short_name;
  device->vendor = _vendor;
  device->version = _version;
  device->extensions = _extensions;
  device->profile = _profile;

  device->global_mem_id = 0;
  device->local_mem_size = L_MEM_SIZE;
  device->image_support = IMAGE_SUPPORT;

  device->max_compute_units = 1;
  device->max_work_group_size = 3;
  device->max_work_item_dimensions = 3;
  device->max_work_item_sizes[0] = \
  device->max_work_item_sizes[1] = \
  device->max_work_item_sizes[2] = 1;
  
  pocl_cgra_data_t *d;
  d = (pocl_cgra_data_t *)calloc (1, sizeof (pocl_cgra_data_t));
  if (d == NULL)
    return CL_OUT_OF_HOST_MEMORY;

  d->available = CL_TRUE;
  device->available = &(d->available);
  device->compiler_available = CL_TRUE;
  device->linker_available = CL_TRUE;
  device->data = (void *)d;

  /* LLVM */
  device->address_bits = 32;
  // LLVM target
  device->llvm_target_triplet = strdup("x86_64-linux-gnu");
  device->llvm_cpu = strdup("x86-64");
  device->extensions = strdup("");
  device->double_fp_config = 0;
  device->preferred_vector_width_double = 0;
  device->native_vector_width_double = 0;

  printf("CGRA::init\n");
  return ret;
}

cl_int pocl_cgra_alloc_mem_obj(cl_device_id device, cl_mem mem_obj, void *host_ptr)
{
  cl_int ret = CL_SUCCESS;
  printf("CGRA::alloc_mem_obj\n");
  printf("device gmemID = %d\n", device->global_mem_id);
  
  /* if we share global memory with CPU, let the CPU driver allocate it */
  if (device->global_mem_id == 0)
    return pocl_driver_alloc_mem_obj (device, mem_obj, host_ptr);

  /* ... otherwise allocate. */
  printf("local allocate mem obj\n");
  pocl_mem_identifier *p = &mem_obj->device_ptrs[device->global_mem_id];
  pocl_global_mem_t *gmem = device->global_memory;
  pocl_cgra_data_t* d = device->data;
  void *b = NULL;
  p->mem_ptr = b;
  p->version = 0;

  if (b == NULL)
    return CL_MEM_OBJECT_ALLOCATION_FAILURE;

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
  snprintf (res, 1000, _hash_str);
  return res;
}

void
pocl_cgra_write (void *data,
                 const void *__restrict__  src_host_ptr,
                 pocl_mem_identifier * dst_mem_id,
                 cl_mem dst_buf,
                 size_t offset, size_t size)
{
  printf("CGRA::write\n");
  pocl_driver_write(data, src_host_ptr, dst_mem_id, dst_buf, offset, size);
}

void
pocl_cgra_read (void *data,
                void *__restrict__  dst_host_ptr,
                pocl_mem_identifier * src_mem_id,
                cl_mem src_buf,
                size_t offset,
                size_t size)
{
  printf("CGRA::read\n");
  pocl_driver_read(data, dst_host_ptr, src_mem_id, src_buf, offset, size);
}

void
pocl_cgra_broadcast (cl_event event)
{
  printf("CGRA::broadcasting\n");
  pocl_broadcast(event);
}

static void
cgra_schedule_command(pocl_cgra_data_t *data)
{
  _cl_command_node *node;

  while ((node = data->ready_list))
  {
      printf("CGRA::schedule_command\n");
      assert (pocl_command_is_ready (node->sync.event.event));
      assert (node->sync.event.event->status == CL_SUBMITTED);
      CDL_DELETE (data->ready_list, node);
      POCL_UNLOCK (data->cq_lock);
      printf("CGRA:exec_in_command\n");
      pocl_exec_command (node);
      printf("CGRA:exec_out_command\n");
      POCL_LOCK (data->cq_lock);
  }
  return;
}

void
pocl_cgra_submit (_cl_command_node *node, cl_command_queue cq)
{
  printf("CGRA::submit-%x-%x\n", node->type, node->command);
  switch (node->type)
  {
	case CL_COMMAND_NDRANGE_KERNEL:
	  printf("CL_COMMAND_NDRANGE_KERNEL\n");
	  break;
    case CL_COMMAND_TASK:
	  printf("CL_COMMAND_TASK\n");
	  break;
	case CL_COMMAND_NATIVE_KERNEL:
	  printf("CL_COMMAND_NATIVE_KERNEL\n");
	  break;
    case CL_COMMAND_READ_BUFFER:
	  printf("CL_COMMAND_READ_BUFFER\n");
	  break;
	case CL_COMMAND_WRITE_BUFFER:
	  printf("CL_COMMAND_WRITE_BUFFER\n");
	  break;
	case CL_COMMAND_COPY_BUFFER:
	  printf("CL_COMMAND_COPY_BUFFER\n");
	  break;
	case CL_COMMAND_MAP_BUFFER:
	  printf("CL_COMMAND_MAP_BUFFER\n");
    default:
      printf("Unknown command type type::%x command::%x\n", node->type, node->command);
	  break;
  }

  if (node->type == CL_COMMAND_NDRANGE_KERNEL)
  {

  }

  pocl_cgra_data_t *data = node->device->data;
  node->state = POCL_COMMAND_READY;
  POCL_LOCK (data->cq_lock);
  pocl_command_push(node, &data->ready_list, &data->command_list);
  POCL_UNLOCK_OBJ (node->sync.event.event);
  cgra_schedule_command(data);
  POCL_UNLOCK (data->cq_lock);
}

void
pocl_cgra_join (cl_device_id device, cl_command_queue cq)
{
  printf("CGRA::join\n");
}

void
pocl_cgra_flush (cl_device_id device, cl_command_queue cq)
{
  printf("CGRA::flush\n");
}

void
pocl_cgra_run (void *data, _cl_command_node *cmd)
{
  printf("CGRA::run\n");
}

int
pocl_cgra_build_source (cl_program program, cl_uint device_i,
      /* these are filled by clCompileProgram(), otherwise NULLs */
      cl_uint num_input_headers, const cl_program *input_headers,
      const char **header_include_names,
      /* 1 = compile & link, 0 = compile only, linked later via clLinkProgram*/
      int link_program)
{

  cl_device_id device = program->devices[device_i];
  int _compile_program = device->compiler_available;
  int _link_program    = device->linker_available;
  printf("CGRA::build_from_source::COMPILE:%d::LINK:%d\n", _compile_program, _link_program);
  return pocl_driver_build_source(program, device_i, num_input_headers, input_headers, header_include_names, _link_program);
}

int pocl_cgra_setup_metadata (
  cl_device_id device, 
  cl_program program, 
  unsigned int program_device_i
)
{
  printf("CGRA::setup_metadata\n");
  return pocl_driver_setup_metadata(device, program, program_device_i);
}

int
pocl_cgra_compile_kernel (
  _cl_command_node *cmd,
  cl_kernel kernel,
  cl_device_id device,
  int specialize
)
{
  printf("CGRA::compile_kernel\n");
  return CL_SUCCESS;
}

