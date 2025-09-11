
set(CGRA_SOURCES "cgra.c"
		 "cgra.h"
)

add_pocl_device_library(pocl-devices-cgra ${CGRA_SOURCES})
