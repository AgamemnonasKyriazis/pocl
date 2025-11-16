#ifndef POCL_CGRA_H
#define POCL_CGRA_H

#include "pocl_cl.h"

#include "prototypes.inc"
GEN_PROTOTYPES (cgra)

struct vcgra_region
{
    unsigned int id;
};

struct vcgra_kernel
{
    unsigned int hash;
    const char *kname;
    struct vcgra_region *regions;
};


#endif /* POCL_CGRA_H */
