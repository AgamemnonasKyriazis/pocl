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

int cgra_write_to_device (const void * __restrict__ host_ptr, size_t size)
{
    if (host_ptr == NULL)
        return -1;
    printf("CGRA::MemWrite To Device %lx - %lu\n", (uint64_t*)host_ptr, size);
    uint32_t wlen = 0;
    wlen = pwrite(usr_fd, (uint64_t*)host_ptr, sizeof(uint64_t)*size, DEVICE_MEM_BASE_ADDR);
    return wlen;
}

int cgra_read_from_device (void * __restrict__ host_ptr, size_t size)
{
    if (host_ptr == NULL)
        return -1;
    printf("CGRA::MemRead From Device %lx - %lu\n", (uint64_t*)host_ptr, size);
    uint32_t rlen = 0;
    rlen = pread(usr_fd, (uint64_t*)host_ptr, sizeof(uint64_t)*size, DEVICE_MEM_BASE_ADDR);
    return rlen;
}