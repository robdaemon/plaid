#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/pc.h>
#include <kernel/kbd.h>

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
  
  printf("Hello world\nThis is the kernel.\nCan you hear me now?");

  for (int i = 0; i < 5; i++) {
	printf("Yet another line: %d.\n", i);
  }

  //  printf("Causing a GPF fault here (firing the interrupt):");

  //  asm volatile ("int $0x3");
  for(;;);
}
