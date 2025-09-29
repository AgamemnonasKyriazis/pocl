#include "cgra_mem.h"

int usr_fd;

uint32_t wbuf[] = {
    0x00000000,
    0x11111111,
    0x22222222,
    0x33333333,
    0x44444444,
    0x55555555,
    0x66666666,
    0x77777777,
    0x88888888,
    0x99999999,
    0xaaaaaaaa,
    0xbbbbbbbb,
    0xcccccccc,
    0xdddddddd,
    0xeeeeeeee,
    0xffffffff,
};

uint32_t rbuf[sizeof(wbuf)] = {0};

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
  
  printf("Write Sanity Check\n");
  for (int i = 0; i < size/(sizeof(uint32_t)); i+=1)
    printf("0x%lx\n", ((uint32_t*)host_ptr)[i]);

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }

  int wlen = pwrite(usr_fd, (uint32_t*)wbuf, size, DEVICE_MEM_BASE_ADDR);
  if (wlen <= 0 && size != 0)
  {
    perror("Write Error");
    return -1;
  }
  
  close(usr_fd);

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

  usr_fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
  if (usr_fd < 0) {
    perror("open user");
    return -1;
  }

  int rlen = pread(usr_fd, (uint32_t*)rbuf, size, DEVICE_MEM_BASE_ADDR);
  if (rlen <= 0 && size != 0)
  {
    perror("Read Error");
    return -1;
  }
  
  close(usr_fd);

  printf("Read Sanity Check\n");
  for (int i = 0; i < size/(sizeof(uint32_t)); i+=1)
    printf("0x%lx\n", ((uint32_t*)rbuf)[i]);
  
  return size;
}