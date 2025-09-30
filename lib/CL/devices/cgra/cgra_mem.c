#include "cgra_mem.h"

const char * h2c_fp = "/dev/xdma0_h2c_0";
const char * c2h_fp = "/dev/xdma0_c2h_0";

int h2c_fd;
int c2h_fd;

int cgra_write_to_device (
    void *data, const void *__restrict__ host_ptr,
    pocl_mem_identifier *dst_mem_id, cl_mem dst_buf,
    size_t offset, size_t size)
{
  uint64_t * ptr = (uint64_t *)host_ptr;
  size_t wsize  = sizeof(uint64_t) * ((size_t)(size/(sizeof(uint64_t))));
  int32_t wret = 0;

  printf("CGRA::MemWrite To Device %lx - %lu\n", ptr, wsize);

  h2c_fd = open(h2c_fp, O_RDWR | O_SYNC);
  if (h2c_fd < 0) {
    perror("open h2c");
    return -1;
  }

  wret = pwrite(h2c_fd, ptr, wsize, DEVICE_MEM_BASE_ADDR);
  if (wret <= 0 && size != 0) {
      perror("Write Error");
      return -1;
  }

  close(h2c_fd);
  return size;
}

int cgra_read_from_device (
    void *data, void *__restrict__ host_ptr,
    pocl_mem_identifier *src_mem_id, cl_mem src_buf,
    size_t offset, size_t size)
{
  uint64_t * ptr = (uint64_t *)host_ptr;
  size_t rsize  = sizeof(uint64_t) * ((size_t)size/(sizeof(uint64_t)));
  int rret = 0;

  printf("CGRA::MemRead From Device %lx - %lu\n", ptr, rsize);

  c2h_fd = open(c2h_fp, O_RDWR | O_SYNC);
  if (c2h_fd < 0) {
    perror("open c2h");
    return -1;
  }

  rret = pread(c2h_fd, ptr, rsize, DEVICE_MEM_BASE_ADDR);
  if (rret <= 0 && size != 0) {
      perror("Read Error");
      return -1;
  }

  close(c2h_fd);
  
  return size;
}