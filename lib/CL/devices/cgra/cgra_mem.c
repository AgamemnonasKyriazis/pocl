#include "cgra_mem.h"

int usr_fd;

int cgra_write_to_device (
    void *data, const void *__restrict__ host_ptr,
    pocl_mem_identifier *dst_mem_id, cl_mem dst_buf,
    size_t offset, size_t size)
{
  printf("CGRA::MemWrite To Device %lx - %lu\n", (uint64_t*)host_ptr, size);

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }
  
  uint32_t wdata = 0x11111111;
  int wlen = pwrite(usr_fd, &wdata, sizeof(uint32_t), DEVICE_MEM_BASE_ADDR);
  if (wlen <= 0 && size != 0)
  {
    perror("Write Error");
    return -1;
  }
  printf("wdata=%x\n", wdata);
  
  close(usr_fd);

  return size;
}

int cgra_read_from_device (
    void *data, void *__restrict__ host_ptr,
    pocl_mem_identifier *src_mem_id, cl_mem src_buf,
    size_t offset, size_t size)
{
  printf("CGRA::MemRead From Device %lx - %lu\n", (uint64_t*)host_ptr, size);

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }

  uint32_t rdata = 0;
  int rlen = pread(usr_fd, &rdata, sizeof(uint32_t), DEVICE_MEM_BASE_ADDR);
  if (rlen <= 0 && size != 0)
  {
    perror("Read Error");
    return -1;
  }
  printf("rdata=%x\n", rdata);

  close(usr_fd);
  
  return size;
}