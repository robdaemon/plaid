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
#include <kernel/arch/i386/task.h>

extern struct multiboot* mboot_ptr;
extern uint32_t placement_address;

uint32_t initial_esp;

void traverse_initrd();

void kernel_early(void) { terminal_initialize(); }

int kernel_main(uint32_t esp) {
  initial_esp = esp;

  gdt_install();
  idt_install();
  isrs_install();
  irq_install();

  asm volatile("sti");

  timer_install();

  keyboard_install();

  ASSERT(mboot_ptr != 0);
  ASSERT(mboot_ptr->mods_count > 0);
  uint32_t initrd_location = *((uint32_t*)mboot_ptr->mods_addr);
  uint32_t initrd_end = *(uint32_t*)(mboot_ptr->mods_addr + 4);
  // move the start of our kernel heap to past the initrd
  placement_address = initrd_end;

  initialize_paging();

  initialize_tasking();

  fs_root = initialize_initrd(initrd_location);

  // Create a new process in a new address space which is a clone of this.
  int ret = fork();

  printf("fork() returned %x, ", ret);
  printf(" getpid() returned %x\n", getpid());

  printf("Loading initial ramdisk...\n");

  asm volatile("cli");
  traverse_initrd();
  asm volatile("sti");

  printf("That's all folks\n");

  for(;;);

  return 0;
}

void traverse_initrd() {
  int i = 0;
  struct dirent* node = 0;
  while ((node = readdir_fs(fs_root, i)) != 0) {
    printf("Found file %s ", node->name);

    fs_node_t* fsnode = finddir_fs(fs_root, node->name);
    if ((fsnode->flags & 0x7) == FS_DIRECTORY) {
      printf("[directory]\n");
    } else {
      printf("[file of %d bytes]\n", fsnode->length);
    }

    i++;
  }
  printf("\n");
}
