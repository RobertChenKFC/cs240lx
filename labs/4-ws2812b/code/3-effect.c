/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"

// the pin used to control the light strip.
enum { pix_pin = 21 };

void enable_cache(void) {
    unsigned r;
    asm volatile ("MRC p15, 0, %0, c1, c0, 0" : "=r" (r));
	r |= (1 << 12); // l1 instruction cache
	r |= (1 << 11); // branch prediction
    asm volatile ("MCR p15, 0, %0, c1, c0, 0" :: "r" (r));
};

// crude routine to write a pixel at a given location.
void place_cursor(neo_t h, int i) {
    neopix_write(h,i-2,0xff,0,0);
    neopix_write(h,i-1,0,0xff,0);
    neopix_write(h,i,0,0,0xff);
    neopix_flush(h);
    neopix_clear(h);
}

uint8_t inter(unsigned s, unsigned t, unsigned i, unsigned n) {
  return (s * (n - i) + t * i) / n;
}

enum {
  MAX_BRIGHTNESS = 0x80
};
void nxt_color(uint8_t *ss, int *pc, int *pd) {
  int c = *pc, d = *pd;
  if (ss[c] + d > MAX_BRIGHTNESS || ss[c] + d < 0) {
    if ((c = ++(*pc)) == 3) {
      d = *pd = -d;
      c = *pc = 0;
    }
  }
  ss[c] += d;
}

void notmain(void) {
    enable_cache(); 
    gpio_set_output(pix_pin);

    // make sure when you implement the neopixel 
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 30;  // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);
    uint8_t ss[] = {MAX_BRIGHTNESS, 0, 0};
    uint8_t tt[] = {0, 0, MAX_BRIGHTNESS};
    int cc[] = {0, 2}, dd[] = {1, -1};
    while (1) {
      nxt_color(ss, &cc[0], &dd[0]);
      nxt_color(tt, &cc[1], &dd[1]);
      for (int i = 0; i < npixels; ++i) {
        neopix_write(h, i,
            inter(ss[0], tt[0], i, npixels),
            inter(ss[1], tt[1], i, npixels),
            inter(ss[2], tt[2], i, npixels));
      }
      neopix_flush(h);
      delay_ms(3);
    }

    output("done!\n");
}
