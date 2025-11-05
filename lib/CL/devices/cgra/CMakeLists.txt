
find_package(LLVM REQUIRED CONFIG)    # or REQUIRED CONFIG 19 if you want to pin

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM cmake dir:   ${LLVM_DIR}")

set(CGRA_SOURCES
	"cgra.c"
	"cgra.h"

	"cgra_codegen.h"
	"cgra_ir_loader.cc"

	"xdma.c"
	"xdma.h"
)

add_pocl_device_library(pocl-devices-cgra ${CGRA_SOURCES})

# Compile as C++
target_compile_features(pocl-devices-cgra PRIVATE cxx_std_17)

# Add LLVM include dirs and defs
target_include_directories(pocl-devices-cgra PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(pocl-devices-cgra PRIVATE ${LLVM_DEFINITIONS})