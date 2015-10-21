#include <stdio.h>
#include <string.h>

#include <kernel/kmalloc.h>

#include <kernel/arch/i386/paging.h>
#include <kernel/arch/i386/kheap.h>
#include <kernel/arch/i386/pc.h>

page_directory_t* kernel_directory = 0;
page_directory_t* current_directory = 0;

uint32_t* frames;
uint32_t nframes;

extern uint32_t placement_address;
extern heap_t* kheap;

extern heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max,
                           uint8_t supervisor, uint8_t readonly);

extern void copy_page_physical(uint32_t src, uint32_t dest);

#define INDEX_FROM_BIT(a) (a / (8 * 4))
#define OFFSET_FROM_BIT(a) (a % (8 * 4))

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
  for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
    if (frames[i] != 0xFFFFFFFF) {
      for (j = 0; j < 32; j++) {
        uint32_t to_test = 0x1 << j;
        if (!(frames[i] & to_test)) {
          return i * 4 * 8 + j;
        }
      }
    }
  }

  return 0;
}

void alloc_frame(page_t* page, int is_kernel, int is_writable) {
  if (page->frame != 0) {
    return;
  } else {
    uint32_t index = first_frame();
    if (index == (uint32_t)-1) {
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
  if (!(frame = page->frame)) {
    return;
  } else {
    clear_frame(frame);
    page->frame = 0x0;
  }
}

void initialize_paging() {
  // TODO: replace with a call to determine total RAM
  // For now, we're using 16 MB
  uint32_t memory_end_page = 0x1000000;

  nframes = memory_end_page / 0x1000;
  frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
  memset(frames, 0, INDEX_FROM_BIT(nframes));

  // Make a page directory
  kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
  memset(kernel_directory, 0, sizeof(page_directory_t));
  kernel_directory->physicalAddr = (uint32_t)kernel_directory->tablesPhysical;

  // Map some pages into the kernel heap.
  uint32_t i = 0;
  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000) {
    get_page(i, 1, kernel_directory);
  }

  // Now, do some identity mapping (physical addr = virtual addr) from 0x0 to
  // the end of used memory, so we can access this transparently as if we
  // weren't
  // paging.
  i = 0;
  while (i < placement_address + 0x1000) {
    // kernel code is readable but not writable from user space.
    alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
    i += 0x1000;
  }

  // Now, allocate the pages we mapped.
  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000) {
    alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
  }

  irq_install_handler(14, page_fault);

  switch_page_directory(kernel_directory);

  // Initialize the kernel heap.
  kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000,
                      0, 0);

  current_directory = clone_directory(kernel_directory);
  switch_page_directory(current_directory);
}

void switch_page_directory(page_directory_t* dir) {
  current_directory = dir;
  asm volatile("mov %0, %%cr3" ::"r"(dir->physicalAddr));
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

static page_table_t* clone_table(page_table_t* src, uint32_t* physAddr) {
  // Make a new, page aligned, page table
  page_table_t* table = (page_table_t*)kmalloc_ap(sizeof(page_table_t), physAddr);
  // Blank the table
  memset(table, 0, sizeof(page_table_t));

  int i;
  for(i = 0; i < 1024; i++) {
    // If the source has a frame attached to it:
    if(src->pages[i].frame) {
      // Get a new frame
      alloc_frame(&table->pages[i], 0, 0);
      // Clone the flags
      if(src->pages[i].present) table->pages[i].present = 1;
      if(src->pages[i].rw) table->pages[i].rw = 1;
      if(src->pages[i].user) table->pages[i].user = 1;
      if(src->pages[i].accessed) table->pages[i].accessed = 1;
      if(src->pages[i].dirty) table->pages[i].dirty = 1;
      // Physically copy the data.
      copy_page_physical(src->pages[i].frame * 0x1000, table->pages[i].frame * 0x1000);
    }
  }

  return table;
}

page_t* get_page(uint32_t address, int make, page_directory_t* dir) {
  address /= 0x1000;
  uint32_t table_index = address / 1024;
  if (dir->tables[table_index]) {
    return &dir->tables[table_index]->pages[address % 1024];
  } else if (make) {
    uint32_t temp;
    dir->tables[table_index] =
        (page_table_t*)kmalloc_ap(sizeof(page_table_t), &temp);
    memset(dir->tables[table_index], 0, 0x1000);
    dir->tablesPhysical[table_index] = temp | 0x7;  // PRESENT, RW, USER
    return &dir->tables[table_index]->pages[address % 1024];
  } else {
    return 0;
  }
}

page_directory_t* clone_directory(page_directory_t* src) {
  uint32_t phys;
  page_directory_t* dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);
  // blank out the directory
  memset(dir, 0, sizeof(page_directory_t));

  uint32_t offset = (uint32_t)dir->tablesPhysical - (uint32_t)dir;

  dir->physicalAddr = phys + offset;

  int i;
  for(i = 0; i < 1024; i++) {
    if(!src->tables[i]) {
      continue;
    }

    if(kernel_directory->tables[i] == src->tables[i]) {
      // page is in the kernel, so just use a pointer
      dir->tables[i] = src->tables[i];
      dir->tablesPhysical[i] = src->tablesPhysical[i];
    } else {
      // Copy the table
      uint32_t phys;
      dir->tables[i] = clone_table(src->tables[i], &phys);
      dir->tablesPhysical[i] = phys | 0x7;
    }
  }

  return dir;
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
  if (present) {
    printf("present ");
  }
  if (rw) {
    printf("read-only ");
  }
  if (us) {
    printf("user-mode ");
  }
  if (reserved) {
    printf("reserved ");
  }
  if (id) {
    printf("ID: %d ", id);
  }
  printf("] at 0x%d\n", faulting_address);
  for (;;)
    ;
}
