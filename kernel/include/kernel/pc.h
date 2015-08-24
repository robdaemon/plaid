#ifndef __KERNEL_PC_H
#define __KERNEL_PC_H

#include <stddef.h>
#include <stdint.h>

typedef struct registers {
  uint32_t ds;                                     // Data segment selector
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha sets these
  uint32_t int_no, err_code;                       // Interrupt number and error code
  uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor
} registers_t;

extern void gdt_flush();
void gdt_install();

extern void idt_load();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_install();

void isrs_install();
void fault_handler(registers_t r);

void irq_install_handler(int32_t irq, void (*handler)(registers_t r));
void irq_uinstall_handler(int32_t irq);
void irq_remap(void);
void irq_install();
void irq_handler(registers_t r);

void timer_install();

static inline void outportb(uint16_t port, uint8_t val) {
  asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inportb(uint16_t port) {
  uint8_t ret;
  asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
  return ret;
}

#endif
