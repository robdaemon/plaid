#ifndef __TASK_H
#define __TASK_H 1

#include <kernel/arch/i386/paging.h>
#include <stdint.h>

typedef struct task {
  int id;        // process ID
  uint32_t esp;  // Stack pointer
  uint32_t ebp;  // Base pointer
  uint32_t eip;  // Instruction pointer
  page_directory_t* page_directory;
  struct task* next;  // Linked list of tasks
} task_t;

void initialize_tasking();

void task_switch();

int fork();

void move_stack(void* new_stack_start, uint32_t size);

int getpid();

#endif