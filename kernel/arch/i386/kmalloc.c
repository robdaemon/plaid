#include <stdint.h>

#include <kernel/pc.h>

extern uint32_t placement_address;

uint32_t kmalloc_int(uint32_t sz, int alignment, uint32_t *phys_addr) {
  if(alignment == 1 && (placement_address & 0xFFFFF000)) {
	placement_address &= 0xFFFFF000;
	placement_address += 0x1000;
  }

  if(phys_addr) {
	*phys_addr = placement_address;
  }

  uint32_t tmp = placement_address;
  placement_address += sz;
  return tmp;
}

uint32_t kmalloc_a(uint32_t sz) {
  return kmalloc_int(sz, 1, 0);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys_addr) {
  return kmalloc_int(sz, 1, phys_addr);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys_addr) {
  return kmalloc_int(sz, 1, phys_addr);
}

uint32_t kmalloc(uint32_t sz) {
  return kmalloc_int(sz, 0, 0);
}
