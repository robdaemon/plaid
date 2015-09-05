#ifndef __KERNEL_FS_H
#define __KERNEL_FS_H 1

#include <stdint.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

struct fs_node;

typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);
typedef struct dirent* (*readdir_type_t)(struct fs_node*, uint32_t);
typedef struct fs_node* (*finddir_type_t)(struct fs_node*, char* name);

typedef struct fs_node {
  char name[128]; // filename. 128 characters isn't sufficient, will need to extend

  // these fields basically implement POSIX filesystems. This isn't
  // the ultimate goal, but is a starting point. I would prefer an
  // ACL-based system.
  uint32_t mask;   // Permissions
  uint32_t uid;    // Owner user
  uint32_t gid;    // Owner group
  uint32_t flags;  // Node type
  uint32_t inode;  // Device-specific, how to find the file
  uint32_t length; // Size of the file in bytes
  uint32_t impl;   // Implementation-defined number

  read_type_t read;
  write_type_t write;
  open_type_t open;
  close_type_t close;
  readdir_type_t readdir;
  finddir_type_t finddir;

  struct fs_node *ptr; // Symbolic links and mountpoints use this
} fs_node_t;

// This defines a POSIX-like filesystem root. Will be changed to a
// different model where there are multiple filesystem roots, one
// per device. More like the Amiga filesystem.
extern fs_node_t* fs_root; // Filesystem root.

// Standard file operation functions.
uint32_t read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void open_fs(fs_node_t* node, uint8_t read, uint8_t write);
void close_fs(fs_node_t* node);
struct dirent* readdir_fs(fs_node_t* node, uint32_t index);
fs_node_t* finddir_fs(fs_node_t* node, char* name);

#endif