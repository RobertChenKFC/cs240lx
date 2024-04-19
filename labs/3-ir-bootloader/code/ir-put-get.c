#include "rpi.h"
#include "rpi-thread.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "ir-put-get.h"

#include "fast-hash32.h"

int abs(int x) {
  return x < 0 ? -x : x;
}

// read a bit:
//   1. record how long IR receiver it down.
//   2. if closer to <burst_0> cycles, return 0.
//   3. if closer to <burst_1> cycles, return 1.
//
// suggested modifications:
//  - panic if read 0 for "too long" (e.g., to catch
//    a bad jumper.
//  - ignore very short 1s (which can happen if the sender
//    screws up transmission timing).
int tsop_read_bit(unsigned pin) {
#ifdef SEND_USE_STAFF
  return staff_tsop_read_bit(pin);
#endif
  while (gpio_read(pin))
    rpi_yield();
  uint32_t s = cycle_cnt_read();
  while (!gpio_read(pin))
    rpi_yield();
  uint32_t t = cycle_cnt_read();
  int d = t - s;
  return abs(d - burst_1) < abs(d - burst_0);
}

void send_burst(unsigned pin, uint32_t burst, uint32_t quiet) {
  uint32_t s = cycle_cnt_read();
  while (cycle_cnt_read() - s < burst) {
    gpio_write(pin, 1);
    uint32_t t = cycle_cnt_read();
    while (cycle_cnt_read() - t < tsop_cycle)
      rpi_yield();
    gpio_write(pin, 0);
    t = cycle_cnt_read();
    while (cycle_cnt_read() - t < tsop_cycle)
      rpi_yield();
  }
  s = cycle_cnt_read();
  while (cycle_cnt_read() - s < quiet)
    rpi_yield();
}

void write_bit(unsigned pin, uint8_t b) {
  if (b)
    send_burst(pin, burst_1, quiet_1);
  else
    send_burst(pin, burst_0, quiet_0);
}

volatile static int lock = 0;
void ir_put8(unsigned pin, uint8_t c) {
#ifdef SEND_USE_STAFF
  staff_ir_put8(pin,c);
  return;
#endif

  while (lock)
    rpi_yield();
  lock = 1;

  for (int i = 0; i < 8; ++i)
    write_bit(pin, (c >> i) & 1);

  lock = 0;
}

uint8_t ir_get8(unsigned pin) {
#ifdef RECV_USE_STAFF
  return staff_ir_get8(pin);
#endif

  uint8_t b = 0;
  for (int i = 0; i < 8; ++i)
    b |= tsop_read_bit(pin) << i;
  return b;
}

void ir_put32(unsigned pin, uint32_t x) {
#ifdef SEND_USE_STAFF
  staff_ir_put32(pin,x);
  return;
#endif

  for (int i = 0; i < 32; i += 8)
    ir_put8(pin, (x >> i) & 0xff);
}

uint32_t ir_get32(unsigned pin) {
#ifdef RECV_USE_STAFF
  return staff_ir_get32(pin);
#endif

  uint32_t x = 0;
  for (int i = 0; i < 32; i += 8)
    x |= ir_get8(pin) << i;
  return x;
}

// bad form to have in/out pins globally.
int ir_send_pkt(void *data, uint32_t nbytes) {
#ifdef SEND_USE_STAFF
  return staff_ir_send_pkt(data,nbytes);
#endif
  uint8_t *arr = (uint8_t*)data;
  uint32_t hash = fast_hash_inc32(data, nbytes, 0);

  // DEBUG
  debug("sender: hash = %d (%x), n = %d\n", hash, hash, nbytes);

  ir_put32(out_pin, PKT_HDR);
  ir_put32(out_pin, hash);
  ir_put32(out_pin, nbytes);
  assert(ir_get32(in_pin) == PKT_HDR_ACK);

  // DEBUG
  debug("sender: received PKT_HDR_ACK, start sending data\n");

  ir_put32(out_pin, PKT_DATA);
  for (int i = 0; i < nbytes; ++i) {
    // DEBUG
    debug("sender: sending byte %d\n", i);

    ir_put8(out_pin, arr[i]);
  }

  assert(ir_get32(in_pin) == PKT_DATA_ACK);
  return nbytes;
}

int ir_recv_pkt(void *data, uint32_t max_nbytes) {
#ifdef RECV_USE_STAFF
  return staff_ir_recv_pkt(data,max_nbytes);
#endif

  uint32_t hdr = ir_get32(in_pin);
  uint32_t hash = ir_get32(in_pin);
  uint32_t n = ir_get32(in_pin);
  assert(hdr == PKT_HDR);

  // DEBUG
  debug("receiver: hash = %d (%x), n = %d\n", hash, hash, n);

  ir_put32(out_pin, PKT_HDR_ACK);
  assert(ir_get32(in_pin) == PKT_DATA);
  uint8_t *arr = (uint8_t*)data;
  for (int i = 0; i < n; ++i)
    arr[i] = ir_get8(in_pin);
  assert(hash == fast_hash_inc32(data, n, 0));

  // DEBUG
  debug("receiver: received all data\n", hash, n);

  ir_put32(out_pin, PKT_DATA_ACK);
  return n;
}
