find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM cmake dir:   ${LLVM_DIR}")

set(CGRA_SHELL_EXT_DIR "${PROJECT_SOURCE_DIR}/../shell")
message(STATUS "CGRA_SHELL_EXT_DIR = ${CGRA_SHELL_EXT_DIR}")

set(CGRA_SOURCES
  cgra.c
  cgra.h
  cgra_codegen.h
  cgra_ir_loader.cc
)

set(CGRA_SHELL_SOURCES
  ${CGRA_SHELL_EXT_DIR}/xdma.c
  ${CGRA_SHELL_EXT_DIR}/ffa.c
  ${CGRA_SHELL_EXT_DIR}/lsu.c
  ${CGRA_SHELL_EXT_DIR}/alu.c
  ${CGRA_SHELL_EXT_DIR}/shell.c
)

add_pocl_device_library(pocl-devices-cgra
  ${CGRA_SOURCES}
  ${CGRA_SHELL_SOURCES}
)

target_compile_features(pocl-devices-cgra PRIVATE cxx_std_17)

# Include directories
target_include_directories(pocl-devices-cgra PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${CGRA_SHELL_EXT_DIR}
)

target_compile_definitions(pocl-devices-cgra PRIVATE ${LLVM_DEFINITIONS})
