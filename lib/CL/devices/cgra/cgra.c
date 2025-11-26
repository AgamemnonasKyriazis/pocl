#include "cgra.h"
#include "cgra_codegen.h"
#include "xdma.h"
#include "lsu.h"
#include "alu.h"
#include "shell.h"
#include "timeit.h"

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
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

#include "pocl_cache.h"
#include "pocl_file_util.h"
#include "pocl_mem_management.h"
#include "pocl_timing.h"
#include "pocl_workgroup_func.h"

#include "pocl_cl.h"
#include "bufalloc.h"

#define MEM_BASE_ADDR 0x10000000UL
#define G_MEM_SIZE (1024*8)
#define IMAGE_SUPPORT CL_FALSE

#define VCGRA_N_REGIONS 2

const char * _long_name  = "Memory Mapped Reconfigurable Accelerator";
const char * _short_name = "cgra";
const char * _vendor     = "PoCL";
const char * _version    = "OpenCL 1.2 PoCL";
const char * _extensions = "\0";
const char * _profile    = "FULL_PROFILE";
const char * _hash_str   = "cgmmra-linux-gnu";

memory_region_t * alloc_regions;

mono_region_t * vcgra_regions;

struct vcgra_kernel
{
    char hash[64];
    const char *kname;
    mono_region *region;
};

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

  cl_device_id device;

} pocl_cgra_data_t;

typedef struct {

  pthread_cond_t event_cond;

  volatile int kernel_completed;

  mono_region_t *assigned_region;

} pocl_cgra_event_data_t;

cl_int cgra_init_memory_region () {
  alloc_regions = (memory_region_t *)calloc(1, sizeof(memory_region_t));
  pocl_init_mem_region(alloc_regions, MEM_BASE_ADDR, G_MEM_SIZE);
  return CL_SUCCESS;
}

cl_int cgra_alloc_buffer (pocl_mem_identifier *p, size_t size) {

  assert(p->mem_ptr == NULL);
  chunk_info_t *chunk = NULL;

  chunk = pocl_alloc_buffer(alloc_regions, size);
  if (chunk == NULL)
    return CL_MEM_OBJECT_ALLOCATION_FAILURE;

  // printf("CGRA::Allocated %zu bytes from 0x%zx\n", size, chunk->start_address);

  p->mem_ptr = (void*)chunk->start_address;
  p->version = 0;
  p->extra = 0;

  return CL_SUCCESS;
}

void
pocl_cgra_init_device_ops(struct pocl_device_ops *ops)
{
  ops->device_name = _short_name;
  ops->probe = pocl_cgra_probe;
  ops->init = pocl_cgra_init;
  ops->build_hash = pocl_cgra_build_hash;
  ops->uninit = pocl_cgra_uninit;

  ops->setup_metadata = pocl_cgra_setup_metadata;
  ops->build_source = pocl_cgra_build_source;
  ops->link_program = NULL;
  ops->build_binary = NULL;
  ops->build_builtin = NULL;
  ops->compile_kernel = pocl_cgra_compile_kernel;
  ops->supports_binary = pocl_driver_supports_binary;
  ops->run = pocl_cgra_run;

  // Control
  ops->submit = pocl_cgra_submit;
  ops->join = pocl_cgra_join;
  ops->flush = pocl_cgra_flush;
  ops->broadcast = pocl_cgra_broadcast;
  ops->wait_event = pocl_cgra_wait_event;


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
  cgra_init_memory_region();
  xdma_init();
  
  device->type = CL_DEVICE_TYPE_ACCELERATOR;
  device->long_name = _long_name;
  device->short_name = _short_name;
  device->vendor = _vendor;
  device->version = _version;
  device->extensions = _extensions;
  device->profile = _profile;

  device->global_mem_id = 0;
  device->global_mem_size = G_MEM_SIZE;
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
  d->device = device;

  /* LLVM */
  device->address_bits = 32;
  // LLVM target
  device->llvm_target_triplet = strdup("x86_64-linux-gnu");
  device->llvm_cpu = strdup("x86-64");
  device->extensions = strdup("");
  device->double_fp_config = 0;
  device->preferred_vector_width_double = 0;
  device->native_vector_width_double = 0;

  /* VCGRA */
  vcgra_regions = malloc(sizeof(mono_region_t)*VCGRA_N_REGIONS);
  vcgra_regions[0].is_configured = 0u;
  vcgra_regions[0].address    = 0x00000;
  vcgra_regions[0].__alu_0.id = 0x00000 | vcgra_regions[0].address;
  vcgra_regions[0].__alu_1.id = 0x01000 | vcgra_regions[0].address;
  vcgra_regions[0].__alu_2.id = 0x02000 | vcgra_regions[0].address;
  vcgra_regions[0].__alu_3.id = 0x03000 | vcgra_regions[0].address;
  vcgra_regions[0].__lsu_0.id = 0x04000 | vcgra_regions[0].address;
  vcgra_regions[0].__lsu_1.id = 0x05000 | vcgra_regions[0].address;
  vcgra_regions[0].__ffa_0.id = 0x06000 | vcgra_regions[0].address;

  vcgra_regions[1].is_configured = 0u;
  vcgra_regions[1].address    = 0x10000;
  vcgra_regions[1].__alu_0.id = 0x00000 | vcgra_regions[1].address;
  vcgra_regions[1].__alu_1.id = 0x01000 | vcgra_regions[1].address;
  vcgra_regions[1].__alu_2.id = 0x02000 | vcgra_regions[1].address;
  vcgra_regions[1].__alu_3.id = 0x03000 | vcgra_regions[1].address;
  vcgra_regions[1].__lsu_0.id = 0x04000 | vcgra_regions[1].address;
  vcgra_regions[1].__lsu_1.id = 0x05000 | vcgra_regions[1].address;
  vcgra_regions[1].__ffa_0.id = 0x06000 | vcgra_regions[1].address;

  return ret;
}

cl_int pocl_cgra_uninit(unsigned j, cl_device_id device)
{
  return CL_SUCCESS;
}

cl_int pocl_cgra_alloc_mem_obj(cl_device_id device, cl_mem mem_obj, void *host_ptr)
{
  cl_int ret = CL_SUCCESS;
  pocl_mem_identifier *p = &mem_obj->device_ptrs[device->global_mem_id];
  pocl_global_mem_t *gmem = device->global_memory;
  pocl_cgra_data_t* d = device->data;
  p->mem_ptr = NULL;
  p->version = 0;
  cgra_alloc_buffer(p, mem_obj->size);

  if (p->mem_ptr == NULL)
    return CL_MEM_OBJECT_ALLOCATION_FAILURE;

  return ret;
}

void pocl_cgra_free(cl_device_id device, cl_mem mem)
{
  pocl_mem_identifier *p = &mem->device_ptrs[device->global_mem_id];
  p->mem_ptr = NULL;
  p->version = 0;
}

cl_int pocl_cgra_map_mem(void *data, pocl_mem_identifier *src_mem_id, cl_mem src_buf, mem_mapping_t *map)
{
  return CL_SUCCESS;
}

char *
pocl_cgra_build_hash (cl_device_id device)
{
  char *res = calloc(1000, sizeof(char));
  snprintf (res, 1000, "%s", _hash_str);
  return res;
}

void
pocl_cgra_write (void *data,
                 const void *__restrict__  src_host_ptr,
                 pocl_mem_identifier * dst_mem_id,
                 cl_mem dst_buf,
                 size_t offset, size_t size)
{
  int err = 0;
  void *__restrict__ device_ptr = dst_mem_id->mem_ptr;
  printf("%p-%p\n", src_host_ptr, device_ptr+offset);
  err = xdma_write_mem((void *)src_host_ptr, size, device_ptr+offset);
  if (err <= 0) {
    perror("Write to CGRA Device Failed");
  }
}

void
pocl_cgra_read (void *data,
                void *__restrict__  dst_host_ptr,
                pocl_mem_identifier * src_mem_id,
                cl_mem src_buf,
                size_t offset,
                size_t size)
{
  int err = 0;
  void *__restrict__ device_ptr = src_mem_id->mem_ptr;
  printf("%p-%p\n", dst_host_ptr, device_ptr+offset);
  err = xdma_read_mem((void *)dst_host_ptr, size, device_ptr+offset);
  if (err <= 0) {
    perror("Read from CGRA Device Failed");
  }
}

void
pocl_cgra_broadcast (cl_event event)
{
  pocl_broadcast(event);
}

void pocl_cgra_wait_event(cl_device_id device, cl_event event) {

  if (event->data == NULL) {
      printf("Warning: event has no CGRA-specific data\n");
      return;
  }

  pocl_cgra_event_data_t *ed = (pocl_cgra_event_data_t *)event->data;
  POCL_LOCK_OBJ(event);
  
  printf("Waiting for event, status: %d\n", event->status);
  while(__poll_kernel_status(ed->assigned_region) != 0) {
      printf("Event not complete, waiting...\n");
      POCL_WAIT_COND(ed->event_cond, event->pocl_lock);
  }
    
  printf("Event completed, status: %d\n", event->status);
  POCL_UNLOCK_OBJ(event);
}

static void
cgra_schedule_command(pocl_cgra_data_t *data)
{
  _cl_command_node *node;

  while ((node = data->ready_list))
  {
      assert (pocl_command_is_ready (node->sync.event.event));
      assert (node->sync.event.event->status == CL_SUBMITTED);
      CDL_DELETE (data->ready_list, node);
      POCL_UNLOCK (data->cq_lock);

      if (node != NULL && node->type == CL_COMMAND_NDRANGE_KERNEL) {
        node->device->ops->compile_kernel(node, node->command.run.kernel, node->device, 1);
      }
      pocl_exec_command (node);

      POCL_LOCK (data->cq_lock);
  }
  return;
}

void
pocl_cgra_submit (_cl_command_node *node, cl_command_queue cq)
{
	printf("%s\n", pocl_command_to_str(node->type));

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
{ }

void
pocl_cgra_flush (cl_device_id device, cl_command_queue cq)
{
  pocl_cgra_data_t *data = (pocl_cgra_data_t *)device->data;
  POCL_LOCK (data->cq_lock);
  cgra_schedule_command (data);
  POCL_UNLOCK (data->cq_lock);
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
  return pocl_driver_build_source(program, device_i, num_input_headers, input_headers, header_include_names, _link_program);
}

int
pocl_cgra_setup_metadata (
  cl_device_id device, 
  cl_program program, 
  unsigned int program_device_i
)
{
  return pocl_driver_setup_metadata(device, program, program_device_i);
}

void
pocl_cgra_write_configuration_file(const char* fp, const char* kname)
{
  mono_region_bitstream configuration = {0};
 
  int fdo = open(fp, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (fdo < 0) {
    perror("Failed to write configuration file ");
  }
  
  if (strncmp(kname, (const char *)"addv", POCL_MAX_PATHNAME_LENGTH) == 0) {
    configuration.__alu_cfg_0.op = ALU_ADD; configuration.__alu_cfg_0.src1 = ALU_SRC_0; configuration.__alu_cfg_0.src2 = ALU_SRC_2; configuration.__alu_cfg_0.dstm = ALU_DST_0;
    configuration.__alu_cfg_1.op = ALU_ADD; configuration.__alu_cfg_1.src1 = ALU_SRC_0; configuration.__alu_cfg_1.src2 = ALU_SRC_1; configuration.__alu_cfg_1.dstm = ALU_DST_1;
    configuration.__alu_cfg_2.op = ALU_ADD; configuration.__alu_cfg_2.src1 = ALU_SRC_0; configuration.__alu_cfg_2.src2 = ALU_SRC_2; configuration.__alu_cfg_2.dstm = ALU_DST_1;
    configuration.__alu_cfg_3.op = ALU_ADD; configuration.__alu_cfg_3.src1 = ALU_SRC_0; configuration.__alu_cfg_3.src2 = ALU_SRC_2; configuration.__alu_cfg_3.dstm = 0;
    configuration.__lsu_cfg_0.src = (0x10000000); configuration.__lsu_cfg_0.src_size = (16-1)*sizeof(DTYPE); configuration.__lsu_cfg_0.dst = (0x10000080); configuration.__lsu_cfg_0.dst_size = (16-1)*sizeof(DTYPE);
    configuration.__lsu_cfg_1.src = (0x10000040); configuration.__lsu_cfg_1.src_size = (16-1)*sizeof(DTYPE); configuration.__lsu_cfg_1.dst = (0x00000000); configuration.__lsu_cfg_1.dst_size = 0;
  }
  else
  if (strncmp(kname, (const char *)"loop", POCL_MAX_PATHNAME_LENGTH) == 0) {
    configuration.__alu_cfg_0.op = ALU_ADD; configuration.__alu_cfg_0.src1 = ALU_SRC_0; configuration.__alu_cfg_0.src2 = ALU_SRC_2; configuration.__alu_cfg_0.dstm = ALU_DST_0;
    configuration.__alu_cfg_1.op = ALU_ADD; configuration.__alu_cfg_1.src1 = ALU_SRC_0; configuration.__alu_cfg_1.src2 = ALU_SRC_2; configuration.__alu_cfg_1.dstm = ALU_DST_1;
    configuration.__alu_cfg_2.op = ALU_ADD; configuration.__alu_cfg_2.src1 = ALU_SRC_0; configuration.__alu_cfg_2.src2 = ALU_SRC_2; configuration.__alu_cfg_2.dstm = 0;
    configuration.__alu_cfg_3.op = ALU_ADD; configuration.__alu_cfg_3.src1 = ALU_SRC_0; configuration.__alu_cfg_3.src2 = ALU_SRC_2; configuration.__alu_cfg_3.dstm = 0;
    configuration.__lsu_cfg_0.src = (0x100000c0); configuration.__lsu_cfg_0.src_size = (16-1)*sizeof(DTYPE); configuration.__lsu_cfg_0.dst = (0x10000100); configuration.__lsu_cfg_0.dst_size = (16-1)*sizeof(DTYPE);
    configuration.__lsu_cfg_1.src = (0x00000000); configuration.__lsu_cfg_1.src_size = 0;                    configuration.__lsu_cfg_1.dst = (0x00000000); configuration.__lsu_cfg_1.dst_size = 0;
  }
  else {

  }
  
  ssize_t wn = write(fdo, &configuration, sizeof(mono_region_bitstream));
  close(fdo);
}

int
pocl_cgra_compile_kernel (
  _cl_command_node *cmd,
  cl_kernel kernel,
  cl_device_id device,
  int specialize
)
{
  if (cmd == NULL || cmd->type != CL_COMMAND_NDRANGE_KERNEL)
    return CL_INVALID_OPERATION;

  char cache_dir[POCL_MAX_PATHNAME_LENGTH];
  pocl_cache_program_path(cache_dir, kernel->program, cmd->program_device_i);
  
  char program_bc_path[POCL_MAX_PATHNAME_LENGTH];

  const char *_bc_fname = "/program.bc";

  snprintf(program_bc_path, POCL_MAX_PATHNAME_LENGTH, "%s%s", cache_dir, _bc_fname);

  char *bc = NULL; size_t bc_size = 0;
  if (pocl_read_file(program_bc_path, (char**)&bc, &bc_size) != 0) {
    perror("CL_BUILD_PROGRAM_FAILURE\n");
    return CL_BUILD_PROGRAM_FAILURE;
  }

  const size_t *LS = cmd->command.run.pc.local_size;            // [lx, ly, lz]
  const size_t *NG = cmd->command.run.pc.num_groups;            // [gx, gy, gz] in WGs
  size_t GWS[3] = { NG[0]*LS[0], NG[1]*LS[1], NG[2]*LS[2] };    // global work-items
  const size_t *GO = cmd->command.run.pc.global_offset;         // optional use
  unsigned WD = cmd->command.run.pc.work_dim;                   // or cmd->command.run.work_dim

  const char *kname = kernel->name;
  pocl_kernel_metadata_t *kmd = kernel->meta;
  cl_program prog = kernel->program;

  char entry[POCL_MAX_PATHNAME_LENGTH];
  snprintf(entry, POCL_MAX_PATHNAME_LENGTH, "%s", kname);

  pocl_kernel_metadata_t *meta = kmd;

  cgra_codegen_inject_params(meta, cmd, bc, bc_size, entry);
  
  char fp[POCL_MAX_PATHNAME_LENGTH];

  snprintf(fp, POCL_MAX_PATHNAME_LENGTH, "%s/%s.cfg", cache_dir, kname);

  pocl_cgra_write_configuration_file(fp, kname);

  return CL_SUCCESS;
}

mono_region_t *
pocl_cgra_schedule_kernel(mono_region_bitstream *bit, const char *kname) {
  for (int i = 0; i < VCGRA_N_REGIONS; i+=1) {
    int is_occupied = vcgra_regions[i].is_configured;
    if (!is_occupied) {
      __configure_mono_region(&vcgra_regions[i], bit);
      printf("kernel %s placed at region %d\n", kname, i);
      vcgra_regions[i].is_configured = 1;
      return &vcgra_regions[i];
    }
  }
  return NULL;
}

void
pocl_cgra_run (void *data, _cl_command_node *cmd)
{
  printf("CGRA::run\n");

  cl_kernel kernel = cmd->command.run.kernel;
  const char *kname = kernel->name;

  const size_t *LS = cmd->command.run.pc.local_size;            // [lx, ly, lz]
  const size_t *NG = cmd->command.run.pc.num_groups;            // [gx, gy, gz] in WGs
  size_t GWS[3] = { NG[0]*LS[0], NG[1]*LS[1], NG[2]*LS[2] };    // global work-items
  const size_t *GO = cmd->command.run.pc.global_offset;         // optional use
  unsigned WD = cmd->command.run.pc.work_dim;   


  char cache_dir[POCL_MAX_PATHNAME_LENGTH];
  pocl_cache_program_path(cache_dir, kernel->program, cmd->program_device_i);
  char fp[POCL_MAX_PATHNAME_LENGTH];

  snprintf(fp, POCL_MAX_PATHNAME_LENGTH, "%s/%s.cfg", cache_dir, kname);

  printf("%s\n", fp);

  int fdi = open(fp, O_RDONLY);
  if (fdi < 0)
    perror("Failed to open configuration file ");
  
  mono_region_bitstream bit;
  read(fdi, &bit, sizeof(bit));
  close(fdi);

  mono_region_t *mregion = pocl_cgra_schedule_kernel(&bit, kname);

  cl_event e = cmd->sync.event.event;
  pocl_cgra_event_data_t *edata = (pocl_cgra_event_data_t*)calloc(sizeof(pocl_cgra_event_data_t), 1);
  POCL_INIT_COND(edata->event_cond);
  edata->kernel_completed = 0;
  edata->assigned_region = mregion;  
  e->data = edata;
}