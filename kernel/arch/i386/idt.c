#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/pc.h>

struct idt_entry {
  uint16_t base_low;
  uint16_t sel;
  uint8_t always0;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].base_low = (base & 0xFFFF);
  idt[num].base_high = (base >> 16) & 0xFFFF;

  idt[num].sel = sel;
  idt[num].flags = flags;
  idt[num].always0 = 0;
}

void idt_install() {
  /* Set up the IDT pointer */
  idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
  idtp.base = (uint32_t)&idt;

  /* Clear out the IDT */
  memset(&idt, 0, sizeof(struct idt_entry) * 256);

  /* Add new ISRs here using idt_set_gate */

  /* Point the CPU at the IDT */
  idt_load();
}
