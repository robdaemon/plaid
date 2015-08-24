## Declare constants used for creating a multiboot header.
.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

## Declare a Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

## Reserve a stack for the initial thread.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
	.skip 16384 # 16 KiB
stack_top:

## Kernel entry point
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp
	
	call kernel_early 			# Initialize the core kernel before running the
								# global constructors.

	call _init 					# Call the global constructors
		
	call kernel_main 			# Transfer control to the main kernel

	cli 						# Hang if kernel_main unexpectedly returns

.Lhang:
	hlt
	jmp .Lhang

.global gdt_flush
.extern gp
gdt_flush:
	lgdt gp 					# Load the GDT with our '_gp' which is a special pointer
	movw $0x0010, %ax			# 0x10 is the offset in the GDT to our data segment
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	ljmp $0x0008, $flush2		# 0x08 is the offset to our code segment, far jump!
flush2:
	ret							# back to the C code.

.global idt_load
.extern idtp
idt_load:
	lidt idtp
	ret

	.macro isr_handler isr
	.global isr\isr
	isr\isr:
	cli
	push $0
	push $\isr
	jmp isr_common_stub
	.endm

	isr_handler 0
	isr_handler 1
	isr_handler 2
	isr_handler 3
	isr_handler 4
	isr_handler 5
	isr_handler 6
	isr_handler 7
	isr_handler 8
	isr_handler 9
	isr_handler 10
	isr_handler 11
	isr_handler 12
	isr_handler 13
	isr_handler 14
	isr_handler 15
	isr_handler 16
	isr_handler 17
	isr_handler 18
	isr_handler 19
	isr_handler 20
	isr_handler 21
	isr_handler 22
	isr_handler 23
	isr_handler 24
	isr_handler 25
	isr_handler 26
	isr_handler 27
	isr_handler 28
	isr_handler 29
	isr_handler 30
	isr_handler 31
	
isr_common_stub:
	pusha						# push edi, esi, ebp, esp, ebx, edx, ecx, eax
	movw %ds, %ax				# lower 16 bits of eax = ds
	push %eax					# save the data segment descriptor

	movw $0x0010, %ax			# load the kernel data segment descriptor
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	call fault_handler

	popl %eax 					# reload the segment descriptor
	movw %ds, %ax
	movw %es, %ax
	movw %fs, %ax
	movw %gs, %ax

	popa						# pops edi, esi, ebp, ...
	addl $8, %esp				# Cleans up the pushed error code and ISR number
	sti
	iret						# pops 5 things at once: CS, EIP, EFLAGS, SS and ESP
	
.size _start, . - _start
