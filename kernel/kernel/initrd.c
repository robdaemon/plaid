#include <kernel/initrd.h>
#include <kernel/kmalloc.h>

#include <string.h>

initrd_header_t* initrd_header;
initrd_file_header_t* file_headers;
fs_node_t* initrd_root;
fs_node_t* initrd_devices;
fs_node_t* root_nodes;
uint32_t root_node_count;

struct dirent dirent;

static uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
  initrd_file_header_t header = file_headers[node->inode];
  
  if(offset > header.length) {
	return 0;
  }

  if(offset+size > header.length) {
	size = header.length - offset;
  }

  memcpy(buffer, (uint8_t*)(header.offset + offset), size);

  return size;
}

static struct dirent* initrd_readdir(fs_node_t* node, uint32_t index) {
  if(node == initrd_root && index == 0) {
	strcpy(dirent.name, "devices");
	dirent.name[strlen("devices")] = 0;
	dirent.inode = 0;
	
	return &dirent;
  }

  if((index - 1) >= root_node_count) {
	return 0;
  }

  strcpy(dirent.name, root_nodes[index - 1].name);
  dirent.name[strlen(root_nodes[index - 1].name)] = 0;
  dirent.inode = root_nodes[index - 1].inode;

  return &dirent;
}

static fs_node_t* initrd_finddir(fs_node_t* node, char* name) {
  if(node == initrd_root && !strcmp(name, "devices")) {
	return initrd_devices;
  }

  for(uint32_t i = 0; i < root_node_count; i++) {
	if(!strcmp(name, root_nodes[i].name)) {
	  return &root_nodes[i];
	}
  }

  return 0;
}

fs_node_t* initialize_initrd(uint32_t location) {
  // set up the pointers to the initial ramdisk
  initrd_header = (initrd_header_t*)location;
  file_headers = (initrd_file_header_t*)(location + sizeof(initrd_header_t));

  // set up the root directory
  initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
  memset(initrd_root, 0, sizeof(fs_node_t));
  strcpy(initrd_root->name, "initrd");
  initrd_root->flags = FS_DIRECTORY;
  initrd_root->readdir = &initrd_readdir;
  initrd_root->finddir = &initrd_finddir;

  // set up the devices directory
  initrd_devices = (fs_node_t*)kmalloc(sizeof(fs_node_t));
  memset(initrd_devices, 0, sizeof(fs_node_t));
  strcpy(initrd_devices->name, "devices");
  initrd_devices->flags = FS_DIRECTORY;
  initrd_devices->readdir = &initrd_readdir;
  initrd_devices->finddir = &initrd_finddir;

  root_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * initrd_header->file_count);
  root_node_count = initrd_header->file_count;

  for(uint32_t i = 0; i < initrd_header->file_count; i++) {
	file_headers[i].offset += location;
	strcpy(root_nodes[i].name, (const char*)&file_headers[i].name);
	root_nodes[i].mask = 0;
	root_nodes[i].uid = 0;
	root_nodes[i].gid = 0;
	root_nodes[i].length = file_headers[i].length;
	root_nodes[i].inode = i;
	root_nodes[i].flags = FS_FILE;
	root_nodes[i].read = &initrd_read;
	root_nodes[i].write = 0;
	root_nodes[i].readdir = 0;
	root_nodes[i].finddir = 0;
	root_nodes[i].open = 0;
	root_nodes[i].close = 0;
	root_nodes[i].impl = 0;
  }

  return initrd_root;
}
