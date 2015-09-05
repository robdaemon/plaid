#include <stdio.h>
#include <stdint.h>

#include <kernel/kbd.h>
#include <kernel/arch/i386/pc.h>

/* kbdus is the US Keyboard layout. */
unsigned char kbdus[128] = {
  0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
  0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
  '*',
  0,   /* alt */
  ' ', /* space bar */
  0,   /* caps lock */
  0,   /* 59 - F1 ... F9 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0,   /* F10 */
  0,   /* Num lock */
  0,   /* Scroll lock */
  0,   /* Home */
  0,   /* Up */
  0,   /* Page Up */
  '-',
  0,   /* 79 - End */
  0,   /* Down */
  0,   /* Page Down */
  0,   /* Insert */
  0,   /* Delete */
  0, 0, 0,
  0,   /* F11 */
  0,   /* F12 */
  0,
};

void keyboard_handler(registers_t r) {
  unsigned char scancode;

  /* Read from the keyboard buffer directly */
  scancode = inportb(0x60);

  /* if the top bit is set, this means a key was released */
  if(scancode & 0x80) {
	
  } else {
	/* Key was pressed */

	/* This is just going to echo the key to the screen */
	putchar(kbdus[scancode]);
  }
}

void keyboard_install() {
  irq_install_handler(1, keyboard_handler);
}
