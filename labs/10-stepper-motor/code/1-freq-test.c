#include "rpi.h"
#include "a4988.h"

// you need to fill these in.
enum { dir_delay = 1, step_delay = 500 };

// rotate shaft 360 degrees.
static void run_circle(step_t *s, int direction) {
    for(int n = 0; n < 200; n++) 
        step(s,direction);
}

// 500 -> 996
// 1000 -> 498
// 2000 -> 249

void notmain(void) {
    demand(dir_delay && step_delay, must set these up);

    enum { dir = 21, step = 20 };

#if 0
    uint32_t delays[] = {
      190,
      170,
      151,
      143,
      127,
      113,
      101,
      95,
      0
    };
    enum {
      N = 2
    };
    for (int j = 0; delays[j]; ++j) {
      step_t s = step_mk(dir, dir_delay, step, delays[j]);
      for(int i = 0; i < N; i++)
        run_circle(&s, forward);
    }

    step_t s = step_mk(dir, dir_delay, step, 200);
    for (int i = 0; i < 30; ++i)
      run_circle(&s, forward);
#endif

    // DEBUG
// #include "1-freq-test.h"
#include "us_anthem.h"
// #include "rick_roll.h"

    int d = forward;
    for (int j = 0; step_delays[j]; ++j) {
      step_t s = step_mk(dir, dir_delay, step, step_delays[j]);
      for(int i = 0; i < step_lengths[j]; i++)
        run_circle(&s, d);
      d = (d == forward) ? forward : backward;
    }

    clean_reboot();
}

