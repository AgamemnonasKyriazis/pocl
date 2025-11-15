#ifndef POCL_CGRA_CODEGEN_H
#define POCL_CGRA_CODEGEN_H

#include <stddef.h>
#include <stdint.h>

#include "pocl_cl.h"

#ifdef __cplusplus
extern "C" {
#endif

int cgra_codegen(const void* bc_data, size_t bc_size, const char* entry);

void cgra_codegen_inject_params(pocl_kernel_metadata_t* meta, _cl_command_node *cmd, const void* bc_data, size_t bc_size, const char* entry);

#ifdef __cplusplus
}
#endif
#endif /* POCL_CGRA_CODEGEN_H */