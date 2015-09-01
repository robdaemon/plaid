#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/pc.h>
#include <kernel/kbd.h>
#include <kernel/paging.h>

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
  __asm__ __volatile__ ("sti");

  uint32_t a = kmalloc(8);
  
  initialize_paging();
  uint32_t b = kmalloc(8);
  uint32_t c = kmalloc(8);
  
  printf("Hello world\nThis is the kernel.\nCan you hear me now?");

  for (int i = 0; i < 5; i++) {
	printf("Yet another line: %d.\n", i);
  }

  printf("a = %d\n", a);
  printf("b = %d\n", b);
  printf("c = %d\n", c);

  kfree(c);
  kfree(b);

  uint32_t d = kmalloc(12);
  printf("d = %d\n", d);
  
  //  printf("Causing a GPF fault here (firing the interrupt):");

  //  asm volatile ("int $0x3");
  for(;;);
}
