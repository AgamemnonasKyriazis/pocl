#include "rc.h"
#include <stdio.h>

void configure_cluster(rc * c, bitstream * configuration) {
    alu_config alu_0_cfg = configuration->alu_0_cfg;
    alu_config alu_1_cfg = configuration->alu_1_cfg;
    alu_config alu_2_cfg = configuration->alu_2_cfg;
    alu_config alu_3_cfg = configuration->alu_3_cfg;
    lsu_config lsu_0_cfg = configuration->lsu_0_cfg;
    lsu_config lsu_1_cfg = configuration->lsu_1_cfg;


    printf(
    "0x%x - alu_0_cfg: op=%u src1=%u src2=%u dstm=%u\n"
    "0x%x - alu_1_cfg: op=%u src1=%u src2=%u dstm=%u\n"
    "0x%x - alu_2_cfg: op=%u src1=%u src2=%u dstm=%u\n"
    "0x%x - alu_3_cfg: op=%u src1=%u src2=%u dstm=%u\n"
    "0x%x - lsu_0_cfg: src=%p dst=%p src_size=%zu dst_size=%zu\n"
    "0x%x - lsu_1_cfg: src=%p dst=%p src_size=%zu dst_size=%zu\n",
    (unsigned)c->alu0.id,
    (unsigned)alu_0_cfg.op,
    (unsigned)alu_0_cfg.src1,
    (unsigned)alu_0_cfg.src2,
    (unsigned)alu_0_cfg.dstm,

    (unsigned)c->alu1.id,
    (unsigned)alu_1_cfg.op,
    (unsigned)alu_1_cfg.src1,
    (unsigned)alu_1_cfg.src2,
    (unsigned)alu_1_cfg.dstm,

    (unsigned)c->alu2.id,
    (unsigned)alu_2_cfg.op,
    (unsigned)alu_2_cfg.src1,
    (unsigned)alu_2_cfg.src2,
    (unsigned)alu_2_cfg.dstm,

    (unsigned)c->alu3.id,
    (unsigned)alu_3_cfg.op,
    (unsigned)alu_3_cfg.src1,
    (unsigned)alu_3_cfg.src2,
    (unsigned)alu_3_cfg.dstm,

    (unsigned)c->lsu0.id,
    (void *)lsu_0_cfg.src,
    (void *)lsu_0_cfg.dst,
    lsu_0_cfg.src_size,
    lsu_0_cfg.dst_size,

    (unsigned)c->lsu1.id,
    (void *)lsu_1_cfg.src,
    (void *)lsu_1_cfg.dst,
    lsu_1_cfg.src_size,
    lsu_1_cfg.dst_size
    );

    configure_alu(c->alu0, alu_0_cfg.op, alu_0_cfg.src1, alu_0_cfg.src2, alu_0_cfg.dstm);
    configure_alu(c->alu1, alu_1_cfg.op, alu_1_cfg.src1, alu_1_cfg.src2, alu_1_cfg.dstm);
    configure_alu(c->alu2, alu_2_cfg.op, alu_2_cfg.src1, alu_2_cfg.src2, alu_2_cfg.dstm);
    configure_alu(c->alu3, alu_3_cfg.op, alu_3_cfg.src1, alu_3_cfg.src2, alu_3_cfg.dstm);

    transfer_from_memory(c->lsu0, lsu_0_cfg.src, lsu_0_cfg.src_size);
    transfer_to_memory(c->lsu0, lsu_0_cfg.dst, lsu_0_cfg.dst_size);
    
    transfer_from_memory(c->lsu1, lsu_1_cfg.src, lsu_1_cfg.src_size);
    transfer_to_memory(c->lsu1, lsu_1_cfg.dst, lsu_1_cfg.dst_size);
}
