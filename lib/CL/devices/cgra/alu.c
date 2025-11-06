#include <stdio.h>
#include "alu.h"

int configure_alu(alu inst, uint32_t op, uint32_t src1, uint32_t src2, uint32_t dstm) {
    xdma_write_reg(op,   (ALU_CONTROL+inst.id));
    xdma_write_reg(src1, (ALU_SRC_SEL_A+inst.id));
    xdma_write_reg(src2, (ALU_SRC_SEL_B+inst.id));
    xdma_write_reg(dstm, (ALU_DST_MASK_D+inst.id));
    return 0;
}

void read_alu_csrs(alu inst) {
    uint32_t val;
    printf("ALU::%d\n",inst.id);
    xdma_read_reg(&val, (ALU_CONTROL+inst.id));     printf("ALU_CONTROL::0x%x\n", val);
    xdma_read_reg(&val, (ALU_STATUS+inst.id));      printf("ALU_STATUS::0x%x\n", val);
    xdma_read_reg(&val, (ALU_SRC_SEL_P+inst.id));   printf("ALU_SRC_SEL_P::0x%x\n", val);
    xdma_read_reg(&val, (ALU_SRC_SEL_A+inst.id));   printf("ALU_SRC_SEL_A::0x%x\n", val);
    xdma_read_reg(&val, (ALU_SRC_SEL_B+inst.id));   printf("ALU_SRC_SEL_B::0x%x\n", val);
    xdma_read_reg(&val, (ALU_DST_MASK_P+inst.id));  printf("ALU_DST_MASK_P::0x%x\n", val);
    xdma_read_reg(&val, (ALU_DST_MASK_D+inst.id));  printf("ALU_DST_MASK_D::0x%x\n", val);
}
