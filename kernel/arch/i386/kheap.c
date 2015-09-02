#include <kernel/kheap.h>
#include <kernel/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/pc.h>

extern uint32_t end;
uint32_t placement_address = (uint32_t)&end;
extern page_directory_t* kernel_directory;
heap_t* kheap = 0;

static int32_t find_smallest_hole(uint32_t size, uint8_t page_align, heap_t* heap) {
  uint32_t iter = 0;
  while(iter < heap->index.size) {
	header_t* header = (header_t*)lookup_ordered_array(iter, &heap->index);
	
	if(page_align > 0) {
	  uint32_t location = (uint32_t)header;
	  int32_t offset = 0;
	  if(((location + sizeof(header_t)) & 0xFFFFF000) != 0) {
		offset = 0x1000 /* page size */ - (location + sizeof(header_t)) % 0x1000;
	  }

	  int32_t hole_size = (int32_t)header->size - offset;
	  if(hole_size >= (int32_t)size) {
		break;
	  }
	} else if(header->size >= size) {
	  break;
	}
	iter++;
  }

  if(iter == heap->index.size) {
	return -1;
  } else {
	return iter;
  }
}

static int8_t header_t_less_than(void* a, void* b) {
  return (((header_t*)a)->size < ((header_t*)b)->size) ? 1 : 0;
}

heap_t* create_heap(uint32_t start_addr, uint32_t end_addr, uint32_t max, uint8_t supervisor, uint8_t readonly) {
  heap_t* heap = (heap_t*)kmalloc(sizeof(heap_t));

  ASSERT(start_addr % 0x1000 == 0);
  ASSERT(end_addr % 0x1000 == 0);

  heap->index = place_ordered_array((void*)start_addr, HEAP_INDEX_SIZE, &header_t_less_than);

  start_addr += sizeof(type_t) * HEAP_INDEX_SIZE;

  if((start_addr & 0xFFFFF000) != 0) {
	start_addr &= 0xFFFFF000;
	start_addr += 0x1000;
  }

  heap->start_address = start_addr;
  heap->end_address = end_addr;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;

  header_t* hole = (header_t*)start_addr;
  hole->size = end_addr-start_addr;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;
  insert_ordered_array((void*)hole, &heap->index);

  return heap;
}

static void expand(uint32_t new_size, heap_t* heap) {
  ASSERT(new_size > heap->end_address - heap->end_address);

  if((new_size & 0xFFFFF000) != 0) {
	new_size &= 0xFFFFF000;
	new_size += 0x1000;
  }
  ASSERT(heap->start_address + new_size <= heap->max_address);

  uint32_t old_size = heap->end_address - heap->start_address;
  uint32_t i = old_size;
  while(i < new_size) {
	alloc_frame(get_page(heap->start_address+i, 1, kernel_directory),
				(heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
	i += 0x1000; /* page size */
  }

  heap->end_address = heap->start_address + new_size;
}

static uint32_t contract(uint32_t new_size, heap_t* heap) {
  ASSERT(new_size < heap->end_address - heap->start_address);

  if(new_size & 0x1000) {
	new_size &= 0x1000;
	new_size += 0x1000;
  }

  if(new_size < HEAP_MIN_SIZE) {
	new_size = HEAP_MIN_SIZE;
  }
  uint32_t old_size = heap->end_address - heap->start_address;
  uint32_t i = old_size - 0x1000;
  while(new_size < i) {
	free_frame(get_page(heap->start_address + i, 0, kernel_directory));
	i -= 0x1000;
  }
  heap->end_address = heap->start_address + new_size;
  return new_size;
}

void* alloc(uint32_t size, uint8_t page_align, heap_t* heap) {
  uint32_t new_size = size + sizeof(header_t) + sizeof(footer_t);
  int32_t iter = find_smallest_hole(new_size, page_align, heap);

  if(iter == -1) {
	uint32_t old_length = heap->end_address - heap->start_address;
	uint32_t old_end_address = heap->end_address;

	// Allocate some more space.
	expand(old_length + new_size, heap);
	uint32_t new_length = heap->end_address - heap->start_address;

	// find the end-most header (in location)
	iter = 0;
	// Index of and value of the end-most header found so far.
	int32_t idx = -1;
	uint32_t value = 0x0;
	while((uint32_t)iter < heap->index.size) {
	  uint32_t temp = (uint32_t)lookup_ordered_array(iter, &heap->index);
	  if(temp > value) {
		value = temp;
		idx = iter;
	  }
	  iter++;
	}

	// Didn't find any headers, so we need to add one.
	if(idx == -1) {
	  header_t* header = (header_t*)old_end_address;
	  header->magic = HEAP_MAGIC;
	  header->size = new_length - old_length;
	  header->is_hole = 1;
	  footer_t* footer = (footer_t*)(old_end_address + header->size - sizeof(footer_t));
	  footer->magic = HEAP_MAGIC;
	  footer->header = header;
	  insert_ordered_array((void*)header, &heap->index);
	} else {
	  // Need to adjust the last header.
	  header_t* header = lookup_ordered_array(idx, &heap->index);
	  header->size += new_length - old_length;
	  // Rewrite the footer.
	  footer_t* footer = (footer_t*)((uint32_t)header + header->size - sizeof(footer_t));
	  footer->header = header;
	  footer->magic = HEAP_MAGIC;
	}

	// We have enough space, so recurse.
	return alloc(size, page_align, heap);
  }

  header_t* orig_hole_header = (header_t*)lookup_ordered_array(iter, &heap->index);
  uint32_t orig_hole_pos = (uint32_t)orig_hole_header;
  uint32_t orig_hole_size = orig_hole_header->size;

  // Determine if we need to split the hole into two parts.
  // is the original size - requested size < overhead of adding a new hole?
  if(orig_hole_size - new_size < sizeof(header_t)+sizeof(footer_t)) {
	// Just increase this hole
	size += orig_hole_size - new_size;
	new_size = orig_hole_size;
  }

  // Do we need to page-align?
  if(page_align && orig_hole_pos & 0xFFFFF000) {
	uint32_t new_location = orig_hole_pos + 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(header_t);
	header_t* hole_header = (header_t*)orig_hole_pos;
	hole_header->size = 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(header_t);
	hole_header->magic = HEAP_MAGIC;
	hole_header->is_hole = 1;
	footer_t* hole_footer = (footer_t*)((uint32_t)new_location - sizeof(footer_t));
	hole_footer->magic = HEAP_MAGIC;
	hole_footer->header = hole_header;
	orig_hole_pos = new_location;
	orig_hole_size = orig_hole_size - hole_header->size;
  } else {
	// We don't need this hole anymore, delete it
	remove_ordered_array(iter, &heap->index);
  }

  // overwite the original header
  header_t* block_header = (header_t*)orig_hole_pos;
  block_header->magic = HEAP_MAGIC;
  block_header->is_hole = 0;
  block_header->size = new_size;
  // and the corresponding footer
  footer_t* block_footer = (footer_t*)(orig_hole_pos + sizeof(header_t) + size);
  block_footer->magic = HEAP_MAGIC;
  block_footer->header = block_header;
  
  // write a new hole after the allocated block if the new whole has a positive size
  if(orig_hole_size - new_size > 0) {
	header_t* hole_header = (header_t*)(orig_hole_pos + sizeof(header_t) + size + sizeof(footer_t));
	hole_header->magic = HEAP_MAGIC;
	hole_header->is_hole = 1;
	hole_header->size = orig_hole_size - new_size;
	footer_t* hole_footer = (footer_t*)((uint32_t)hole_header + orig_hole_size - new_size - sizeof(footer_t));
	if((uint32_t)hole_footer < heap->end_address) {
	  hole_footer->magic = HEAP_MAGIC;
	  hole_footer->header = hole_header;
	}
	// Put the new hole into the index
	insert_ordered_array((void*)hole_header, &heap->index);
  }

  // voila!
  return (void*)((uint32_t)block_header + sizeof(header_t));
}

void free(void* p, heap_t* heap) {
  // Null pointers are safe.
  if(p == 0) {
	return;
  }

  header_t* header = (header_t*)((uint32_t)p - sizeof(header_t));
  footer_t* footer = (footer_t*)((uint32_t)header + header->size - sizeof(footer_t));

  ASSERT(header->magic == HEAP_MAGIC);
  ASSERT(footer->magic == HEAP_MAGIC);

  // Make a hole.
  header->is_hole = 1;

  // add this to the 'free holes' index?
  // char so we only use one byte here
  char do_add = 1;

  // Unify left if the item immediately left is a footer
  footer_t* test_footer = (footer_t*)((uint32_t)header - sizeof(footer_t));
  if(test_footer->magic == HEAP_MAGIC && test_footer->header->is_hole == 1) {
	uint32_t cache_size = header->size;
	header = test_footer->header;
	footer->header = header;
	header->size += cache_size;
	do_add = 0;
  }

  // Unify right if the item immediately right is a header
  header_t* test_header = (header_t*)((uint32_t)footer + sizeof(footer_t));
  if(test_header->magic == HEAP_MAGIC && test_header->is_hole) {
	header->size += test_header->size;
	test_footer = (footer_t*)((uint32_t)test_header + test_header->size - sizeof(footer_t));
	footer = test_footer;
	// find and remove this header from the index;
	uint32_t iter = 0;
	while((iter < heap->index.size) &&
		  (lookup_ordered_array(iter, &heap->index) != (void*)test_header)) {
	  iter++;
	}

	ASSERT(iter < heap->index.size);
	remove_ordered_array(iter, &heap->index);
  }

  // If the footer's location is the end of the heap, we can contract
  if((uint32_t)footer + sizeof(footer_t) == heap->end_address) {
	uint32_t old_length = heap->end_address - heap->start_address;
	uint32_t new_length = contract((uint32_t)header - heap->start_address, heap);

	// Check what the size will be when we're done
	if(header->size - (old_length-new_length) > 0) {
	  // We're still here, so resize.
	  header->size -= old_length - new_length;
	  footer = (footer_t*)((uint32_t)header + header->size - sizeof(footer_t));
	  footer->magic = HEAP_MAGIC;
	  footer->header = header;
	} else {
	  // We're gone, so remove it from the index
	  uint32_t iter = 0;
	  while((iter < heap->index.size) &&
			(lookup_ordered_array(iter, &heap->index) != (void*)test_header)) {
		iter++;
	  }

	  if(iter < heap->index.size) {
		remove_ordered_array(iter, &heap->index);
	  }
	}
  }

  // Add us to the index, if necessary
  if(do_add == 1) {
	insert_ordered_array((void*)header, &heap->index);
  }
}
