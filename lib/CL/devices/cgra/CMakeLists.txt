
set(CGRA_SOURCES
	"cgra.c"
	"cgra.h"
	"cgra_mem.c"
	"cgra_mem.h"
)

add_pocl_device_library(pocl-devices-cgra ${CGRA_SOURCES})
