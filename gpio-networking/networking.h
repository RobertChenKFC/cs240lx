#ifndef NETWORKING_H
#define NETWORKING_H

enum {
  NET_CLK = 22,
  NET_TX = 23,
  NET_RCLK = 24,

  NET_CLK_DEFAULT = 0,

  GPIO_BASE = 0x20200000,
  gpio_fsel0 = (GPIO_BASE + 0x00),
  gpio_set0  = (GPIO_BASE + 0x1C),
  gpio_clr0  = (GPIO_BASE + 0x28),
  gpio_lev0  = (GPIO_BASE + 0x34),
};

#define read_pin(pin) ((*((volatile uint32_t*)gpio_lev0) >> pin) & 1)
#define set_pin(pin) (*((volatile uint32_t*)gpio_set0) = 1 << (pin))
#define clr_pin(pin) (*((volatile uint32_t*)gpio_clr0) = 1 << (pin))
#define write_pin(pin, v) \
  do { \
    if (v) \
      set_pin(pin); \
    else \
      clr_pin(pin); \
  } while (0)

#endif
