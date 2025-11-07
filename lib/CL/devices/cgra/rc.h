#ifndef __RC__
#define __RC__

#include <stdint.h>

#include "lsu.h"
#include "alu.h"
#include "ffa.h"

typedef struct {
    uint32_t id;
    lsu lsu0;
    lsu lsu1;
    alu alu0;
    alu alu1;
    alu alu2;
    alu alu3;
    ffa ffa0;
} rc;

typedef struct {
    alu_config alu_0_cfg;
    alu_config alu_1_cfg;
    alu_config alu_2_cfg;
    alu_config alu_3_cfg;
    lsu_config lsu_0_cfg;
    lsu_config lsu_1_cfg;
} bitstream;


void configure_cluster(rc * c, bitstream * configuration);

#endif /* __RC__ */
