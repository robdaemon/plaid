#include <stdio.h>
#include <string.h>

#include <kernel/kmalloc.h>

#include <kernel/arch/i386/paging.h>
#include <kernel/arch/i386/kheap.h>
#include <kernel/arch/i386/pc.h>

page_directory_t* kernel_directory = 0;
page_directory_t* current_directory = 0;

uint32_t *frames;
uint32_t nframes;

extern uint32_t placement_address;
extern heap_t* kheap;

extern heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

static void set_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / 0x1000;
  uint32_t index = INDEX_FROM_BIT(frame);
  uint32_t offset = OFFSET_FROM_BIT(frame);
  frames[index] |= (0x1 << offset);
}

static void clear_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / 0x1000;
  uint32_t index = INDEX_FROM_BIT(frame);
  uint32_t offset = OFFSET_FROM_BIT(frame);
  frames[index] &= ~(0x1 << offset);
}

/* static uint32_t test_frame(uint32_t frame_addr) { */
/*   uint32_t frame = frame_addr / 0x1000; */
/*   uint32_t index = INDEX_FROM_BIT(frame); */
/*   uint32_t offset = OFFSET_FROM_BIT(frame); */
/*   return (frames[index] & (0x1 << offset)); */
/* } */

static uint32_t first_frame() {
  uint32_t i, j;
  for(i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if(frames[i] != 0xFFFFFFFF) {
          for(j = 0; j < 32; j++) {
                uint32_t to_test = 0x1 << j;
                if(!(frames[i] & to_test)) {
                  return i * 4 * 8 + j;
                }
          }
        }
  }

  return 0;
}

void alloc_frame(page_t* page, int is_kernel, int is_writable) {
  if(page->frame != 0) {
        return;
  } else {
        uint32_t index = first_frame();
        if(index == (uint32_t)-1) {
          // panic here
        }
        set_frame(index * 0x1000);
        page->present = 1;
        page->rw = (is_writable) ? 1 : 0;
        page->user = (is_kernel) ? 0 : 1;
        page->frame = index;
  }
}

void free_frame(page_t* page) {
  uint32_t frame;
  if(!(frame = page->frame)) {
        return;
  } else {
        clear_frame(frame);
        page->frame = 0x0;
  }
}

void initialize_paging() {
  //TODO: replace with a call to determine total RAM
  // For now, we're using 16 MB
  uint32_t memory_end_page = 0x1000000;

  nframes = memory_end_page / 0x1000;
  frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
  memset(frames, 0, INDEX_FROM_BIT(nframes));

  kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
  memset(kernel_directory, 0, sizeof(page_directory_t));
  current_directory = kernel_directory;

  // Map some pages into the kernel heap.
  uint32_t i = 0;
  for(i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000) {
        get_page(i, 1, kernel_directory);
  }

  // Now, do some identity mapping (physical addr = virtual addr) from 0x0 to
  // the end of used memory, so we can access this transparently as if we weren't
  // paging.
  i = 0;
  while(i < placement_address + 0x1000) {
        // kernel code is readable but not writable from user space.
        alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
  }

  // Now, allocate the pages we mapped.
  for(i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000) {
        alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
  }

  irq_install_handler(14, page_fault);

  switch_page_directory(kernel_directory);

  // Initialize the kernel heap.
  kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);
}

void switch_page_directory(page_directory_t* dir) {
  current_directory = dir;
  asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
  uint32_t cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t* get_page(uint32_t address, int make, page_directory_t* dir) {
  address /= 0x1000;
  uint32_t table_index = address / 1024;
  if(dir->tables[table_index]) {
        return &dir->tables[table_index]->pages[address % 1024];
  } else if(make) {
        uint32_t temp;
        dir->tables[table_index] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &temp);
        memset(dir->tables[table_index], 0, 0x1000);
        dir->tablesPhysical[table_index] = temp | 0x7; // PRESENT, RW, USER
        return &dir->tables[table_index]->pages[address % 1024];
  } else {
        return 0;
  }
}

void page_fault(registers_t r) {
  uint32_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

  int present = !(r.err_code & 0x1);
  int rw = r.err_code & 0x2;
  int us = r.err_code & 0x4;
  int reserved = r.err_code & 0x8;
  int id = r.err_code & 0x10;

  printf("Page fault detected! [");
  if(present) {
        printf("present ");
  }
  if(rw) {
        printf("read-only ");
  }
  if(us) {
        printf("user-mode ");
  }
  if(reserved) {
        printf("reserved ");
  }
  if(id) {
        printf("ID: %d ", id);
  }
  printf("] at 0x%d\n", faulting_address);
  for(;;);
}
