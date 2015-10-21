#include <stdio.h>

#if defined(__is_plaid_kernel)
#include <kernel/tty.h>
#endif

int putchar(int ic) {
#if defined(__is_plaid_kernel)
  char c = (char)ic;
  terminal_write(&c, sizeof(c));
#else
// TODO: Implement this syscall
#endif

  return ic;
}
