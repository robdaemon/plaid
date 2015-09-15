#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/kbd.h>
#include <kernel/kmalloc.h>
#include <kernel/fs.h>
#include <kernel/initrd.h>

#include <kernel/arch/i386/multiboot.h>
#include <kernel/arch/i386/pc.h>
#include <kernel/arch/i386/paging.h>

extern struct multiboot *mboot_ptr;
extern uint32_t placement_address;

void traverse_initrd();

void kernel_early(void) {
  terminal_initialize();
}

void kernel_main(void) {
  gdt_install();
  idt_install();
  isrs_install();
  irq_install();
  timer_install();
  keyboard_install();
  asm volatile ("sti");

  printf("Loading initial ramdisk...\n");
  
  ASSERT(mboot_ptr != 0);
  ASSERT(mboot_ptr->mods_count > 0);
  uint32_t initrd_location = *((uint32_t*)mboot_ptr->mods_addr);
  uint32_t initrd_end = *(uint32_t*)(mboot_ptr->mods_addr + 4);
  // move the start of our kernel heap to past the initrd
  placement_address = initrd_end;
  
  uint32_t a = kmalloc(8);
  
  initialize_paging();
  uint32_t b = kmalloc(8);
  uint32_t c = kmalloc(8);
  
  printf("Hello world\nThis is the kernel.\nCan you hear me now?");

  for (int i = 0; i < 5; i++) {
	printf("Yet another line: %d.\n", i);
  }

  printf("a = %x\n", a);
  printf("b = %x\n", b);
  printf("c = %x\n", c);

  kfree((void*)c);
  kfree((void*)b);

  uint32_t d = kmalloc(12);
  printf("d = %x\n", d);

  fs_root = initialize_initrd(initrd_location);
  traverse_initrd();
  
  //  printf("Causing a GPF fault here (firing the interrupt):");

  //  asm volatile ("int $0x3");
  for(;;);
}

void traverse_initrd() {
  int i = 0;
  struct dirent* node = 0;
  while((node = readdir_fs(fs_root, i)) != 0) {
	printf("Found file %s ", node->name);

	fs_node_t* fsnode = finddir_fs(fs_root, node->name);
	if((fsnode->flags & 0x7) == FS_DIRECTORY) {
	  printf("[directory]\n");
	} else {
	  printf("[file of %d bytes]\n", fsnode->length);
	}

	i++;
  }
}

