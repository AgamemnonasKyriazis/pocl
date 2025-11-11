#include "xdma.h"

#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

const char * h2c_0_fp = "/dev/xdma0_h2c_0";
const char * h2c_1_fp = "/dev/xdma0_h2c_1";
const char * h2c_2_fp = "/dev/xdma0_h2c_2";
const char * h2c_3_fp = "/dev/xdma0_h2c_3";

const char * c2h_0_fp = "/dev/xdma0_c2h_0";
const char * c2h_1_fp = "/dev/xdma0_h2c_1";
const char * c2h_2_fp = "/dev/xdma0_h2c_2";
const char * c2h_3_fp = "/dev/xdma0_h2c_3";

const char * usr_fp   = "/dev/xdma0_user";

int h2c_0_fd = -1;
int h2c_1_fd = -1;
int h2c_2_fd = -1;
int h2c_3_fd = -1;

int c2h_0_fd = -1;
int c2h_1_fd = -1;
int c2h_2_fd = -1;
int c2h_3_fd = -1;

int usr_fd   = -1;

int xdma_init_done = 0;

int xdma_init () {
    if (xdma_init_done) {
        return 0;
    }
    h2c_0_fd = open (h2c_0_fp, O_WRONLY | O_SYNC);
    if (h2c_0_fd < 0) {
        perror ("Failed to open W-CH0 of XDMA");
        return -1;
    }
    c2h_0_fd = open (c2h_0_fp, O_RDONLY);
    if (c2h_0_fd < 0) {
        perror ("Failed to open R-CH0 of XDMA");
        return -1;
    }
    usr_fd = open (usr_fp, O_RDWR | O_SYNC);
    if (usr_fd < 0) {
        perror ("Failed to open RW-CHu of XDMA");
        return -1;
    }
    xdma_init_done = 1;
    return 0;
}

int xdma_release() {
    close(h2c_0_fd);
    close(c2h_0_fd);
    close(usr_fd);
    xdma_init_done = 0;
    return xdma_init_done;
}

int xdma_write_mem (void * hptr, size_t nbytes, void * dptr) {
    if (h2c_0_fd < 0) {
        perror ("Failed to use W-CHx of XDMA file not open");
        return -1;
    }
    
    if (nbytes & 0b11) {
        perror ("Unaligned size");
        return -1;
    }
    
    if (((uint64_t)dptr) & 0b11) {
        perror ("Unaligned pointer to device");
        return -1;
    }
    
    if (nbytes == 0) {
        return 0;
    }
    
    int bytes_written = pwrite (h2c_0_fd, (char*)hptr, nbytes, (uint64_t)dptr);
    
    if (bytes_written <= 0) {
        perror ("Write failed - No data written to device");
        return -1;
    }

    return bytes_written;
}

int xdma_read_mem (void * hptr, size_t nbytes, void * dptr) {
    if (c2h_0_fd < 0) {
        perror ("Failed to use R-CHx of XDMA file not open");
        return -1;
    }
    if (nbytes & 0b11) {
        perror ("Unaligned size");
        return -1;
    }
    if (((uint64_t)dptr) & 0b11) {
        perror ("Unaligned pointer to device");
        return -1;
    }
    if (nbytes == 0) {
        return 0;
    }
    int bytes_read = pread (c2h_0_fd, (char*)hptr, nbytes, (uint64_t)dptr);
    if (bytes_read <= 0) {
        perror ("Read failed - No data read from device");
        return -1;
    }

    return bytes_read;
}

int xdma_write_reg (uint32_t value, void * regptr) {
    if (usr_fd < 0) {
        perror ("Failed to use RW-CHu of XDMA file not open");
        return -1;
    }
    if (((uint32_t)regptr) & 0b11) {
        perror ("Unaligned pointer to device");
        return -1;
    }
    int bytes_written = pwrite (usr_fd, &value, sizeof(uint32_t), (uint32_t)regptr);
    if (bytes_written <= 0) {
        perror ("Write failed - No data written to device");
        return -1;
    }
    return bytes_written;
}

int xdma_read_reg (uint32_t * buf, void * regptr) {
    if (usr_fd < 0) {
        perror ("Failed to use RW-CHu of XDMA file not open");
        return -1;
    }
    if (((uint32_t)regptr) & 0b11) {
        perror ("Unaligned pointer to device");
        return -1;
    }
    int bytes_read = pread (usr_fd, buf, sizeof(uint32_t), (uint32_t)regptr);
    if (bytes_read <= 0) {
        perror ("Write failed - No data written to device");
        return -1;
    }
    return bytes_read;
}
