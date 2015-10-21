#include <kernel/arch/i386/task.h>
#include <kernel/kmalloc.h>
#include <string.h>

// Currently running task
volatile task_t* current_task;

// Start of the task queue list
volatile task_t* ready_queue;

extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
extern void alloc_frame(page_t*, int, int);
extern uint32_t initial_esp;
extern uint32_t read_eip();

// Next process ID
uint32_t next_pid = 1;

void initialize_tasking() {
  // disable interrupts, this would cause havoc
  asm volatile("cli");

  // Relocate the stack frame
  move_stack((void*)0xE0000000, 0x2000);

  // Initialize the first task (kernel)
  current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
  current_task->id = next_pid++;
  current_task->esp = 0;
  current_task->ebp = 0;
  current_task->eip = 0;
  current_task->page_directory = current_directory;
  current_task->next = 0;

  // Reenable interrupts
  asm volatile("sti");
}

void move_stack(void* new_stack_start, uint32_t size) {
  uint32_t i;
  // Allocate a new stack
  for (i = (uint32_t)new_stack_start; i >= ((uint32_t)new_stack_start - size);
       i -= 0x1000) {
    alloc_frame(get_page(i, 1, current_directory), 0, /* user mode */
                1 /* writable */);
  }

  // Flush TLB
  uint32_t pd_addr;
  asm volatile("mov %%cr3, %0" : "=r"(pd_addr));
  asm volatile("mov %0, %%cr3" : : "r"(pd_addr));

  // Old ESP and EBP come from registers
  uint32_t old_esp;
  asm volatile("mov %%esp, %0" : "=r"(old_esp));
  uint32_t old_ebp;
  asm volatile("mov %%ebp, %0" : "=r"(old_ebp));

  // Offset to get a new stack
  uint32_t offset = (uint32_t)new_stack_start - initial_esp;

  // New ESP and EBP
  uint32_t new_esp = old_esp + offset;
  uint32_t new_ebp = old_ebp + offset;

  // Copy stack
  memcpy((void*)new_esp, (void*)old_esp, initial_esp - old_esp);

  // Backtrack through the stack, copying to the new stack
  for (i = (uint32_t)new_stack_start; i > (uint32_t)new_stack_start - size;
       i -= 4) {
    uint32_t tmp = *(uint32_t*)i;

    // If the value is inside the old stack, assume it's a base pointer and
    // remap.
    if ((old_esp < tmp) && (tmp < initial_esp)) {
      tmp = tmp + offset;
      uint32_t* tmp2 = (uint32_t*)i;
      *tmp2 = tmp;
    }
  }

  asm volatile("mov %0, %%esp" : : "r"(new_esp));
  asm volatile("mov %0, %%ebp" : : "r"(new_ebp));
}

void switch_task() {
  // Tasking isn't initialized yet, so don't do anything
  if(!current_task) {
    return;
  }

  // Read esp, ebp and eip so we can save them.
  uint32_t esp, ebp;
  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%ebp, %0" : "=r"(ebp));

  // Read the instruction pointer.
  // Two possibile conditions - 
  // 1 - we get a real EIP, so we're all good.
  // 2 - we get a fake pointer, which means we've returned from being switched on
  uint32_t eip;
  eip = read_eip();

  // This is the magic number that indicates we just switched tasks
  if(eip == 0x12345) {
    return;
  }

  // Nope, haven't switched yet. Let's set up the registers.
  current_task->eip = eip;
  current_task->esp = esp;
  current_task->ebp = ebp;

  // Get the next task.
  current_task = current_task->next;
  // if we walk off the end of the list, go back to the beginning.
  // Note: This means there is no priority queue. Tasks are truly
  // round-robined.
  if(!current_task) {
    current_task = ready_queue;
  }

  eip = current_task->eip;
  esp = current_task->esp;
  ebp = current_task->ebp;

  // Change the page directory appropriately
  current_directory = current_task->page_directory;

  // Now, set these registers, but do it in a block that is effectively "atomic"
  asm volatile(
      "                  \
    cli;                 \
    mov %0, %%ecx;       \
    mov %1, %%esp;       \
    mov %2, %%ebp;       \
    mov %3, %%cr3;       \
    mov $0x12345, %%eax; \
    sti;                 \
    jmp *%%ecx           "
      :
      : "r"(eip), "r"(esp), "r"(ebp), "r"(current_directory->physicalAddr));
}

int fork() {
  // disable interrupts - an interrupt can cause havoc here
  asm volatile("cli");

  // Get a pointer to the task struct for later reference
  task_t* parent_task = (task_t*)current_task;

  // Clone the address space
  page_directory_t* directory = clone_directory(current_directory);

  // Create a new process
  task_t* new_task = (task_t*)kmalloc(sizeof(task_t));

  new_task->id = next_pid++;
  new_task->esp = 0;
  new_task->ebp = 0;
  new_task->eip = 0;
  new_task->page_directory = directory;
  new_task->next = 0;

  // Add to the end of the queue
  task_t* tmp_task = (task_t*)ready_queue;
  while(tmp_task->next) {
    tmp_task = tmp_task->next;
  }
  tmp_task->next = new_task;

  // Entry point for the new process
  uint32_t eip = read_eip();

  // Check if we're the parent or child task here
  if(current_task == parent_task) {
    // we're the parent, set up the child
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    uint32_t ebp;
    asm volatile("mov %%ebp, %0" : "=r"(ebp));

    new_task->esp = esp;
    new_task->ebp = ebp;
    new_task->eip = eip;
    asm volatile("sti");

    return new_task->id;
  } else {
    // We're the child, so we have nothing left to do.
    return 0;
  }
}

int getpid() {
  return current_task->id;
}