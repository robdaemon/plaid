#include <stdio.h>
#include <stdint.h>

#include <kernel/arch/i386/pc.h>

int32_t timer_ticks = 0;

void timer_handler(__attribute__((unused)) registers_t r) {
  timer_ticks++;

  if (timer_ticks % 18 == 0) {
	printf("One second! Ticks: %d\n", (int)timer_ticks);
  }
}

void timer_install() {
  irq_install_handler(0, timer_handler);
}
