#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/arch/i386/pc.h>

struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access,
                  uint8_t gran) {
  /* Set the base address */
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;

  /* Set the descriptior limits */
  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = ((limit >> 16) & 0x0F);

  /* Set the granularity and access flags */
  gdt[num].granularity |= (gran & 0xF0);
  gdt[num].access = access;
}

void gdt_install() {
  gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
  gp.base = (uint32_t)&gdt;

  /* Set up the NULL descriptor */
  gdt_set_gate(0, 0, 0, 0, 0);

  /* Set up the code segment. Base = 0, limit = 4 GiB, with 4 KiB granularity,
     32-bit opcodes, and is a code segment descriptor.
   */
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  /* Set up the data segment. Same as the code segment */
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  /* Flush out the old GDT, loading ours */
  gdt_flush();
}
