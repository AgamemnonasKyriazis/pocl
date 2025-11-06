#ifndef __ALU__
#define __ALU__

#include "xdma.h"
#include <stdint.h>

#define ALU_CONTROL     0x0000
#define ALU_STATUS      0x0004
#define ALU_SRC_SEL_P   0x0008
#define ALU_SRC_SEL_A   0x000C
#define ALU_SRC_SEL_B   0x0010
#define ALU_DST_MASK_P  0x0014
#define ALU_DST_MASK_D  0x0018

#define ALU_ADD     0x00
#define ALU_SUB     0x01
#define ALU_AND     0x02
#define ALU_OR      0x03
#define ALU_XOR     0x04
#define ALU_SRL     0x05
#define ALU_SLL     0x06
#define ALU_MUL     0x07
#define ALU_PHI     0x08
#define ALU_EQ      0x09
#define ALU_NE      0x0a
#define ALU_LT      0x0b
#define ALU_GT      0x0c
#define ALU_LTE     0x0b
#define ALU_GTE     0x0d

#define ALU_SRC_0   0x00
#define ALU_SRC_1   0x01

#define ALU_DST_0   0x01
#define ALU_DST_1   0x02

typedef struct
{
    uint32_t id;
} alu;

typedef struct
{
    uint32_t op;
    uint32_t src1;
    uint32_t src2;
    uint32_t dstm;
} alu_config;


int configure_alu(alu inst, uint32_t op, uint32_t src1, uint32_t src2, uint32_t dstm);

void read_alu_csrs(alu inst);

#endif /* __ALU__ */
