#include <kernel/ordered_array.h>
#include <kernel/kmalloc.h>
#include <kernel/arch/i386/pc.h>

#include <string.h>

int8_t standard_lessthan_predicate(type_t a, type_t b) {
  return (a < b) ? 1 : 0;
}

ordered_array_t create_ordered_array(uint32_t max_size,
                                     lessthan_predicate_t less_than) {
  ordered_array_t ret;
  ret.array = (void*)kmalloc(max_size * sizeof(type_t));
  memset(ret.array, 0, max_size * sizeof(type_t));
  ret.size = 0;
  ret.max_size = max_size;
  ret.less_than = less_than;
  return ret;
}

ordered_array_t place_ordered_array(void* addr, uint32_t max_size,
                                    lessthan_predicate_t less_than) {
  ordered_array_t ret;
  ret.array = (type_t*)addr;
  memset(ret.array, 0, max_size * sizeof(type_t));
  ret.size = 0;
  ret.max_size = max_size;
  ret.less_than = less_than;
  return ret;
}

void destroy_ordered_array(ordered_array_t* array) { kfree(array->array); }

void insert_ordered_array(type_t item, ordered_array_t* array) {
  ASSERT(array->less_than);
  uint32_t iter = 0;
  while (iter < array->size && array->less_than(array->array[iter], item)) {
    iter++;
  }

  if (iter == array->size) {
    array->array[array->size++] = item;
  } else {
    type_t temp = array->array[iter];
    array->array[iter] = item;
    while (iter < array->size) {
      iter++;
      type_t temp2 = array->array[iter];
      array->array[iter] = temp;
      temp = temp2;
    }
    array->size++;
  }
}

type_t lookup_ordered_array(uint32_t i, ordered_array_t* array) {
  ASSERT(i < array->size);
  return array->array[i];
}

void remove_ordered_array(uint32_t i, ordered_array_t* array) {
  while (i < array->size) {
    array->array[i] = array->array[i + 1];
    i++;
  }
  array->size--;
}
