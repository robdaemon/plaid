#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/vga.h>
#include <kernel/tty.h>
#include <kernel/pc.h>

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = make_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
  terminal_buffer = VGA_MEMORY;
  for(size_t y = 0; y < VGA_HEIGHT; y++) {
    for(size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = make_vgaentry(' ', terminal_color);
    }
  }
}

void terminal_setcolor(uint8_t color) {
  terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = make_vgaentry(c, color);
}

void terminal_putchar(char c) {
  if (c && c == '\n') {
	terminal_row++;
	terminal_column = 0;
	terminal_scroll();
  } else {
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if(++terminal_column == VGA_WIDTH) {
	  terminal_column = 0;
	  terminal_row++;

	  terminal_scroll();
	}
  }

  terminal_move_cursor();
}

void terminal_write(const char* data, size_t size) {
  for(size_t i = 0; i < size; i++) {
    terminal_putchar(data[i]);
  }
}

void terminal_writestring(const char* data) {
  terminal_write(data, strlen(data));
}

void terminal_scroll(void) {
  unsigned blank, temp;

  blank = 0x20 | (terminal_color << 8);

  if(terminal_row >= VGA_HEIGHT) {
	/* Move the current text chunk in the buffer by one line */
	temp = terminal_row - VGA_HEIGHT + 1;
	memcpy(terminal_buffer,
		   terminal_buffer + temp * VGA_WIDTH, (VGA_HEIGHT - temp) * VGA_WIDTH * 2);

	/* Set the last row to our blank character */
	memsetw(terminal_buffer + (VGA_HEIGHT - temp) * VGA_WIDTH, blank, VGA_WIDTH);
	terminal_row = VGA_HEIGHT - 1;
  }
}

void terminal_move_cursor(void) {
  unsigned temp;

  temp = terminal_row * VGA_WIDTH + terminal_column;

  outportb(0x3D4, 14);
  outportb(0x3D5, temp >> 8);
  outportb(0x3D4, 15);
  outportb(0x3D5, temp);
}
