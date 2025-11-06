#include "rc.h"

void configure_cluster(rc * c, bitstream * configuration) {
    alu_config alu_0_cfg = configuration->alu_0_cfg;
    alu_config alu_1_cfg = configuration->alu_1_cfg;
    alu_config alu_2_cfg = configuration->alu_2_cfg;
    alu_config alu_3_cfg = configuration->alu_3_cfg;
    lsu_config lsu_0_cfg = configuration->lsu_0_cfg;
    lsu_config lsu_1_cfg = configuration->lsu_1_cfg;

    configure_alu(c->alu0, alu_0_cfg.op, alu_0_cfg.src1, alu_0_cfg.src2, alu_0_cfg.dstm);
    configure_alu(c->alu1, alu_1_cfg.op, alu_1_cfg.src1, alu_1_cfg.src2, alu_1_cfg.dstm);
    configure_alu(c->alu2, alu_2_cfg.op, alu_2_cfg.src1, alu_2_cfg.src2, alu_2_cfg.dstm);
    configure_alu(c->alu3, alu_3_cfg.op, alu_3_cfg.src1, alu_3_cfg.src2, alu_3_cfg.dstm);

    transfer_from_memory(c->lsu0, lsu_0_cfg.src, lsu_0_cfg.src_size);
    transfer_to_memory(c->lsu0, lsu_0_cfg.dst, lsu_0_cfg.dst_size);
    transfer_from_memory(c->lsu1, lsu_1_cfg.src, lsu_1_cfg.src_size);
    transfer_to_memory(c->lsu0, lsu_1_cfg.dst, lsu_1_cfg.dst_size);
}
