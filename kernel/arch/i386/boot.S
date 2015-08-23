# Declare constants used for creating a multiboot header.
.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# Declare a Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
	.skip 16384 # 16 KiB
stack_top:

# Kernel entry point
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	# Initialize the core kernel before running the global constructors.
	call kernel_early

	# Call the global constructors
	call _init
		
	# Transfer control to the main kernel
	call kernel_main

	# Hang if kernel_main unexpectedly returns
	cli

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
	
.size _start, . - _start
