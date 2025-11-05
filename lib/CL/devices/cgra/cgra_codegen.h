// C ABI for your C++ codegen
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Error codes ---
typedef enum {
  CGRA_OK = 0,
  CGRA_ERR_INVALID_ARG = -1,
  CGRA_ERR_PARSE_BC    = -2,
  CGRA_ERR_CODEGEN     = -3,
  CGRA_ERR_INTERNAL    = -4
} cgra_status_t;

// --- Opaque handles owned by the C++ side ---
typedef struct cgra_dfg       cgra_dfg_t;       // optional: DFG handle (opaque)

// ------------- Primary API --------------

// Build DFG from a WGF bitcode file. (Optional step, but useful for debugging.)
int cgra_dump_dfg_from_wgf_bc(const void* bc_data, size_t bc_size,
                          const char* entry, const char* dot_out_path);

// Destroy
void cgra_dfg_destroy(cgra_dfg_t* dfg);

#ifdef __cplusplus
}
#endif
