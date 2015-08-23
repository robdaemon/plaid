#ifndef __KERNEL_PC_H
#define __KERNEL_PC_H

#include <stddef.h>
#include <stdint.h>

extern void gdt_flush();
void gdt_install();

static inline void outportb(uint16_t port, uint8_t val) {
  asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inportb(uint16_t port) {
  uint8_t ret;
  asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
  return ret;
}

#endif
