#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void print(const char* data, size_t data_length) {
  for(size_t i = 0; i < data_length; i++) {
	putchar((int)((const unsigned char*)data)[i]);
  }
}

int printf(const char* restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);

  unsigned char *where, buf[16];
  
  int written = 0;
  size_t amount;
  bool rejected_bad_specifier = false;

  while(*format != '\0') {
	if(*format != '%') {
	print_c:
	  amount = 1;
	  while(format[amount] && format[amount] != '%') {
		amount++;
	  }
	  print(format, amount);
	  format += amount;
	  written += amount;
	  continue;
	}

	const char* format_begun_at = format;

	if(*(++format) == '%') {
	  goto print_c;
	}

	if(rejected_bad_specifier) {
	incomprehensible_conversion:
	  rejected_bad_specifier = true;
	  format = format_begun_at;
	  goto print_c;
	}

	if(*format == 'c') {
	  format++;
	  char c = (char)va_arg(parameters, int /* char promotes to int */);
	  print(&c, sizeof(c));
	} else if(*format == 's') {
	  format++;
	  const char* s = va_arg(parameters, const char*);
	  print(s, strlen(s));
	} else if(*format == 'd') {
	  format++;
	  int i = va_arg(parameters, int);

	  where = buf + 16 - 1;
	  *where = '\0';
	  int radix = 10;

	  do {
		unsigned long temp;

		temp = (unsigned long)i % radix;
		where--;
		if(temp < 10) {
		  *where = temp + '0';
		} else {
		  *where = temp - 10 + 'a';
		}
		i = (unsigned long)i / radix;
	  } while(i != 0);

	  print(where, strlen(where));
	} else {
	  goto incomprehensible_conversion;
	}
  }

  va_end(parameters);

  return written;
}
