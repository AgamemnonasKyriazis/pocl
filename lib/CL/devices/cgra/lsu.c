#include "lsu.h"
#include "xdma.h"
#include <stdio.h>

int transfer_from_memory(lsu inst, DTYPE * src, size_t size) {
    uint32_t src_addr_l, src_addr_u;
    uint32_t length;

    if (!xdma_init_done) {
        return -1;
    }

    src_addr_l = ((uint64_t)src) & ((uint64_t)0x0ffffffff);
    src_addr_u = ((uint64_t)src) >> (uint64_t)32;
    length     = (size);

    // xdma_write_reg(DMA_STOP,    (MM2S_DMACR+inst.id));

    xdma_write_reg(src_addr_l,  (MM2S_SA+inst.id));
    xdma_write_reg(src_addr_u,  (MM2S_SA_MSB+inst.id));
    xdma_write_reg(length,      (MM2S_LENGTH+inst.id));
    xdma_write_reg(DMA_START,   (MM2S_DMACR+inst.id));
    return 0;
}

int transfer_to_memory(lsu inst, DTYPE * dst, size_t size) {
    uint32_t dst_addr_l, dst_addr_u;
    uint32_t length;

    if (!xdma_init_done) {
        return -1;
    }

    dst_addr_l = ((uint64_t)dst) & ((uint64_t)0x0ffffffff);
    dst_addr_u = ((uint64_t)dst) >> (uint64_t)32;
    length     = (size);

    // xdma_write_reg(DMA_STOP,    (S2MM_DMACR+inst.id));

    xdma_write_reg(dst_addr_l,  (S2MM_DA+inst.id));
    xdma_write_reg(dst_addr_u,  (S2MM_DA_MSB+inst.id));
    xdma_write_reg(length,      (S2MM_LENGTH+inst.id));
    xdma_write_reg(DMA_START,   (S2MM_DMACR+inst.id));
    return 0;
}

void read_lsu_csrs(lsu inst) {
    uint32_t val;
    printf("LSU::%d\n",inst.id);
    xdma_read_reg(&val, (S2MM_DMACR+inst.id));  printf("S2MM_DMACR::0x%x\n", val);
    xdma_read_reg(&val, (S2MM_DMASR+inst.id));  printf("S2MM_DMASR::0x%x\n", val);
    xdma_read_reg(&val, (S2MM_DA_C+inst.id));   printf("S2MM_DA_C::0x%x\n", val);
    xdma_read_reg(&val, (S2MM_DA+inst.id));     printf("S2MM_DA::0x%x\n", val);
    xdma_read_reg(&val, (S2MM_DA_MSB+inst.id)); printf("S2MM_DA_MSB::0x%x\n", val);
    xdma_read_reg(&val, (S2MM_LENGTH+inst.id));  printf("S2MM_S2MM_LENGTH::0x%x\n", val);

    xdma_read_reg(&val, (MM2S_DMACR+inst.id));  printf("MM2S_DMACR::0x%x\n", val);
    xdma_read_reg(&val, (MM2S_DMASR+inst.id));  printf("MM2S_DMASR::0x%x\n", val);
    xdma_read_reg(&val, (MM2S_SA_C+inst.id));   printf("MM2S_SA_C::0x%x\n", val);
    xdma_read_reg(&val, (MM2S_SA+inst.id));     printf("MM2S_SA::0x%x\n", val);
    xdma_read_reg(&val, (MM2S_SA_MSB+inst.id)); printf("MM2S_SA_MSB::0x%x\n", val);
    xdma_read_reg(&val, (MM2S_LENGTH+inst.id)); printf("MM2S_LENGTH::0x%x\n",val);
}
