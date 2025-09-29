#include "cgra_mem.h"

int usr_fd;

int cgra_write_to_device (
    void *data, const void *__restrict__ host_ptr,
    pocl_mem_identifier *dst_mem_id, cl_mem dst_buf,
    size_t offset, size_t size)
{
  uint32_t * ptr = (uint32_t *)host_ptr;
  uint32_t wlen  = size/(sizeof(uint32_t));
  int ret = 0;

  printf("CGRA::MemWrite To Device %lx - %lu\n", (uint64_t*)host_ptr, size);

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }

  for (int i = 0; i < wlen; i+=1) {
    printf("0x%0x\n", *(ptr+i))
    ret = pwrite(usr_fd, ptr+i, sizeof(uint32_t), DEVICE_MEM_BASE_ADDR+(i*4));
    if (ret <= 0 && size != 0) {
      perror("Write Error");
      return -1;
    }
  }
  
  close(usr_fd);

  return size;
}

int cgra_read_from_device (
    void *data, void *__restrict__ host_ptr,
    pocl_mem_identifier *src_mem_id, cl_mem src_buf,
    size_t offset, size_t size)
{
  uint32_t * ptr = (uint32_t *)host_ptr;
  uint32_t rlen  = size/(sizeof(uint32_t));
  int ret = 0;

  printf("CGRA::MemRead From Device %lx - %lu\n", (uint64_t*)host_ptr, size);

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }

  for (int i = 0; i < rlen; i+=1) {
    ret = pread(usr_fd, ptr+i, sizeof(uint32_t), DEVICE_MEM_BASE_ADDR+(i*4));
    printf("0x%x\n", *(ptr+i));
    if (ret <= 0 && size != 0) {
      perror("Read Error");
      return -1;
    }
  }

  close(usr_fd);
  
  return size;
}