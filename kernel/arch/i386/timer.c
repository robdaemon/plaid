#include <stdint.h>

#include <kernel/pc.h>

int32_t timer_ticks = 0;

void timer_handler(registers_t r) {
  timer_ticks++;

  if (timer_ticks % 18 == 0) {
	printf("One second! Ticks: %d\n", timer_ticks);
  }
}

void timer_install() {
  irq_install_handler(0, timer_handler);
}
