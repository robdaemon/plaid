#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>

void kernel_early(void) {
  terminal_initialize();
}

void kernel_main(void) {
  printf("Hello world\nThis is the kernel.\nCan you hear me now?");

  for (int i = 0; i < 5; i++) {
	printf("Yet another line: %d.\n", i);
  }
}
