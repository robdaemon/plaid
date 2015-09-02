#include <kernel/pc.h>

inline void outportb(uint16_t port, uint8_t val) {
  asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

inline uint8_t inportb(uint16_t port) {
  uint8_t ret;
  asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
  return ret;
}

void panic_assert(const char* file, uint32_t line, const char* desc) {
  asm volatile("cli");

  printf("ASSERTION FAILED: %s at %s:%d\n", desc, file, line);
  for(;;);
}
