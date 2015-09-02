#include <kernel/pc.h>

void panic_assert(const char* file, uint32_t line, const char* desc) {
  asm volatile("cli");

  printf("ASSERTION FAILED: %s at %s:%d\n", desc, file, line);
  for(;;);
}
