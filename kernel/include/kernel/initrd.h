#ifndef __KERNEL_INITRD_H
#define __KERNEL_INITRD_H 1

#include <kernel/fs.h>

typedef struct {
  uint32_t file_count;  // number of files in the ramdisk image
} initrd_header_t;

typedef struct {
  uint8_t magic;    // magic number for consistency check
  int8_t name[64];  // filename
  uint32_t offset;  // offset in the initrd file
  uint32_t length;  // length of the file
} initrd_file_header_t;

fs_node_t* initialize_initrd(uint32_t location);

#endif
