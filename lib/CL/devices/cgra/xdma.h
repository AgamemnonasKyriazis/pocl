#ifndef __XDMA__
#define __XDMA__

#include <stddef.h>
#include <stdint.h>

#define DTYPE uint32_t

extern int xdma_init_done;

int xdma_init ();

int xdma_release();

int xdma_write_mem (void * hptr, size_t nbytes, void * dptr);

int xdma_read_mem (void * hptr, size_t nbytes, void * dptr);

int xdma_write_reg (uint32_t value, void * regptr);

int xdma_read_reg (uint32_t * buf, void * regptr);

#endif /* __XDMA__ */
