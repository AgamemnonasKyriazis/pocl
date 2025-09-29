#include "cgra_mem.h"

int usr_fd;
int h2c_fd;
int c2h_fd;

int cgra_mem_init ()
{
  printf("CGRA::Memory Init\n");
  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }
  
  h2c_fd = open("/dev/xdma0_h2c_0", O_WRONLY | O_DSYNC);
  if (h2c_fd < 0) {
    perror("open h2c");
    return -2;
  }

  c2h_fd = open("/dev/xdma0_c2h_0", O_RDONLY);
  if (c2h_fd < 0) {
    perror("open c2c");
    return -3;
  }
  printf("CGRA::Memory Init - Done\n");
  return 0;

}

int cgra_mem_stream_to_device (void * host_ptr, size_t size)
{
    if (host_ptr == NULL)
        return -1;
    printf("CGRA::Stream To Device %lx - %lu\n", (uint64_t*)host_ptr, size);
    uint32_t wlen = 0;
    wlen = write(h2c_fd, (uint64_t*)host_ptr, sizeof(uint64_t)*size);
    return wlen;
}

int cgra_stream_from_device (void * host_ptr, size_t size)
{
    if (host_ptr == NULL)
        return -1;
    printf("CGRA::Stream From Device %lx - %lu\n", (uint64_t*)host_ptr, size);
    uint32_t rlen = 0;
    rlen = read(c2h_fd, (uint64_t*)host_ptr, sizeof(uint64_t)*size);
    return rlen;
}

int cgra_write_to_device (
    void *data, const void *__restrict__ host_ptr,
    pocl_mem_identifier *dst_mem_id, cl_mem dst_buf,
    size_t offset, size_t size)
{
  if (host_ptr == NULL)
    return -1;
  
  void *__restrict__ device_ptr = dst_mem_id->mem_ptr;

  if (host_ptr == device_ptr)
    return size;

  printf("CGRA::MemWrite To Device %lx - %lu\n", (uint64_t*)host_ptr, size);
  uint32_t wlen = pwrite(usr_fd, (uint32_t*)(host_ptr), size, DEVICE_MEM_BASE_ADDR);
  
//   for (int i = 0; i < size/(sizeof(uint32_t)); i+=1)
//     printf("0x%lx\n", ((uint32_t*)host_ptr)[i]);
  
  return size;
}

int cgra_read_from_device (
    void *data, void *__restrict__ host_ptr,
    pocl_mem_identifier *src_mem_id, cl_mem src_buf,
    size_t offset, size_t size)
{
  if (host_ptr == NULL)
    return -1;
  
  void *__restrict__ device_ptr = src_mem_id->mem_ptr;

  if (host_ptr == device_ptr)
    return size;

  printf("CGRA::MemRead From Device %lx - %lu\n", (uint64_t*)host_ptr, size);
  uint32_t rlen = pread(usr_fd, (uint32_t*)((char *)device_ptr + offset), size, DEVICE_MEM_BASE_ADDR);
  
//   for (int i = 0; i < size/(sizeof(uint32_t)); i+=1)
//     printf("0x%lx\n", ((uint32_t*)((char *)device_ptr + offset))[i]);
  
  return size;
}