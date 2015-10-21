#ifndef __KERNEL_PAGING_H
#define __KERNEL_PAGING_H 1

#include <stdint.h>
#include <kernel/arch/i386/pc.h>

typedef struct page {
  uint32_t present : 1;
  uint32_t rw : 1;
  uint32_t user : 1;
  uint32_t accessed : 1;
  uint32_t dirty : 1;
  uint32_t unused : 7;
  uint32_t frame : 20;
} page_t;

typedef struct page_table { page_t pages[1024]; } page_table_t;

typedef struct page_directory {
  page_table_t* tables[1024];

  uint32_t tablesPhysical[1024];

  uint32_t physicalAddr;
} page_directory_t;

void initialize_paging();
void switch_page_directory(page_directory_t* new);
page_t* get_page(uint32_t address, int make, page_directory_t* dir);
void page_fault(registers_t r);

void alloc_frame(page_t* page, int is_kernel, int is_writable);
void free_frame(page_t* page);

page_directory_t* clone_directory(page_directory_t* src);

#endif
