#include "rpi.h"
#include "networking.h"
#include "fast-hash32.h"
#include "cycle-util.h"

static inline uint8_t net_recv1(void) {
  static int clk = NET_CLK_DEFAULT;

  while (read_pin(NET_CLK) == clk);
  uint8_t b = read_pin(NET_TX);
  clk = !clk;
  write_pin(NET_RCLK, clk);
  return b;
}

static inline uint8_t net_recv8(void) {
  uint8_t x = 0;
  for (int i = 7; i >= 0; --i)
    x |= net_recv1() << i;
  return x;
}

static inline uint32_t net_recv32(void) {
  uint32_t x = 0;
  for (int i = 24; i >= 0; i -= 8)
    x |= (uint32_t)net_recv8() << i;
  return x;
}

uint32_t net_recv(uint8_t *buf) {
  uint32_t len = net_recv32();
  uint32_t ref_hash = net_recv32();
  for (uint32_t i = 0; i < len; ++i)
    buf[i] = net_recv8();
  uint32_t hash = fast_hash(buf, len);
  assert(hash == ref_hash);
  return len;
}

void notmain(void) {
  gpio_set_input(NET_CLK);
  gpio_set_input(NET_TX);
  gpio_set_output(NET_RCLK);

  // Synchronize and start timer
  net_recv1();
  uint32_t s = timer_get_usec_raw();

  char buf[1024];
  uint32_t len = net_recv((uint8_t*)buf);

  uint32_t t = timer_get_usec_raw();
  uint32_t us = t - s;
  uint32_t bits = len * 8 * 1e6 / us;
  debug("Received %u bytes in %u us (%u bit/s, %u byte/s)\n",
        len, us, bits, bits / 8);
}
