#include "rpi.h"
#include "networking.h"
#include "fast-hash32.h"

void net_send1(uint8_t b) {
  static int clk = NET_CLK_DEFAULT;

  clk = !clk;
  write_pin(NET_TX, b);
  write_pin(NET_CLK, clk);
  while (read_pin(NET_RCLK) != clk);
}

void net_send8(uint8_t x) {
  for (int i = 7; i >= 0; --i)
    net_send1((x >> i) & 1);
}

void net_send32(uint32_t x) {
  for (int i = 24; i >= 0; i -= 8)
    net_send8((x >> i) & 0xff);
}

void net_send(const uint8_t *buf, uint32_t len) {
  net_send32(len);
  net_send32(fast_hash(buf, len));
  for (uint32_t i = 0; i < len; ++i)
    net_send8(buf[i]);
}

void notmain(void) {
  gpio_set_output(NET_CLK);
  gpio_set_output(NET_TX);
  gpio_set_input(NET_RCLK);
  gpio_write(NET_CLK, NET_CLK_DEFAULT);

  uint8_t *buf = (uint8_t*)0x8000;
  uint32_t len = 1e3;

  debug("length = %u\n", len);

  // Synchronization
  net_send1(1);
  
  net_send(buf, len);
}
