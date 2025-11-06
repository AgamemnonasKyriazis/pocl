#ifndef __FFA__
#define __FFA__

#include <stdint.h>

#define DEVICE_STATUS_OFFSET               0x0000
#define DEVICE_COMMAND_OFFSET              0x0200
#define DEVICE_CLASS_OFFSET                0x0300
#define DEVICE_ID_OFFSET                   0x0304
#define INTERFACE_TYPE_OFFSET              0x0308
#define CORE_COUNT_OFFSET                  0x030C
#define CTRL_SIZE_OFFSET                   0x0310
#define IMEM_SIZE_OFFSET                   0x0314
#define IMEM_STARTING_ADDRESS_OFFSET       0x0318
#define CQMEM_SIZE_OFFSET                  0x0320
#define CQMEM_STARTING_ADDRESS_OFFSET      0x0328
#define BUFFERMEM_SIZE_OFFSET              0x0330
#define BUFFERMEM_STARTING_ADDRESS_OFFSET  0x0338
#define FEATURE_FLAGS_OFFSET               0x0340

typedef struct
{
    uint32_t id;
} ffa;

#endif /* __FFA__ */