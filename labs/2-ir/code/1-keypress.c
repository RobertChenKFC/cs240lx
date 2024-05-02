// very dumb starter code.  you should rewrite and customize.
//
// when done i would suggest pulling it out into an device source/header 
// file and put in libpi/src so can use later.

#include "rpi.h"
enum { ir_eps = 200 };

// we should never get this.
enum { NOISE = 0 } ;

struct readings { unsigned usec, v; };

const char *key_to_str(unsigned x) {
  switch (x) {
    case 0x45ba00ff:
      return "1";
    case 0x46b900ff:
      return "2";
    case 0x47b800ff:
      return "3";
    case 0x44bb00ff:
      return "4";
    case 0x40bf00ff:
      return "5";
    case 0x43bc00ff:
      return "6";
    case 0x7f800ff:
      return "7";
    case 0x15ea00ff:
      return "8";
    case 0x9f600ff:
      return "9";
    case 0x16e900ff:
      return "*";
    case 0x19e600ff:
      return "0";
    case 0xdf200ff:
      return "#";
    case 0x18e700ff:
      return "up";
    case 0x8f700ff:
      return "left";
    case 0x1ce300ff:
      return "ok";
    case 0x5aa500ff:
      return "right";
    case 0x52ad00ff:
      return "down";
    default:
      return "unknown";
  }
}

// adapt your read_while_equal: return 0 if timeout passed, otherwise
// the number of microseconds + 1 (to prevent 0).
static int read_while_eq(int pin, int v, unsigned timeout) {
    unsigned start = timer_get_usec_raw();
    while(1) {
        // make sure always return != 0
        if(gpio_read(pin) != v)
            return timer_get_usec_raw() - start + 1;
        // unless timeout.
        if((timer_get_usec_raw() - start) >= timeout)
            return 0;
    }
}

// integer absolute value.
static int abs(int x) {
    return x < 0 ? -x : x; 
}

// return 0 if e is closer to <lb>, 1 if its closer to <ub>
static int pick(struct readings *e, unsigned lb, unsigned ub) {
  return abs(e->usec - lb) <= abs(e->usec - ub);
}

// return 1 if is a skip: skip = delay of 550-/+eps
static int is_skip(struct readings *e) {
  return e->v == 0 && abs(e->usec - 550) <= ir_eps;
}

// header is a delay of 9000 and then a delay of 4500
int is_header(struct readings *r, unsigned n) {
  if(n < 2)
      return 0;
  return r[0].v == 0 && r[1].v == 1 &&
         abs(r[0].usec - 9000) <= ir_eps && abs(r[1].usec - 4500) <= ir_eps;
}

// convert <r> into an integer by or'ing in 0 or 1 depending on the 
// time value.
//
// assert that they are seperated by skips!
unsigned convert(struct readings *r, unsigned n) {
  unsigned ret = 0;
  for (int i = 0; i + 1 < n; i += 2) {
    assert(is_skip(&r[i]));
    ret |= pick(&r[i + 1], 600, 1600) << (i / 2);
  }
  return ret;
}

static void print_readings(struct readings *r, int n) {
    printk("-------------------------------------------------------\n");
    for(int i = 0; i < n; i++) {
        printk("\t%d: %d = %d usec\n", i, r[i].v, r[i].usec);
    }
}

// read in values until we get a timeout, return the number of readings.  
static int get_readings(int in, struct readings *r, unsigned N) {
  int n = 0;
  int usec, v = 1;
  while (n < N && (usec = read_while_eq(in, v, 60000))) {
    r[n++] = (struct readings){
      .usec = usec,
      .v = v
    };
    v = !v;
  }
  return n;
}

// initialize the pin.
int tsop_init(int input) {
    // is open hi or lo?  have to set pullup or pulldown
    gpio_set_input(input);
    gpio_set_pullup(input);     // default = 1, so we pull up.
    return 1;
}

void notmain(void) {
    int in = 21;
    tsop_init(in);
    output("about to start reading\n");

    // very dumb starter code
    while(1) {
        // wait until signal: or should we timeout?
        while(gpio_read(in))
            ;
#       define N 256
        static struct readings r[N];
        int n = get_readings(in, r, N);

        output("done getting readings\n");

        // Skip ahead until there is a header
        struct readings *arr = r;
        while (!is_header(arr, n)) {
          ++arr;
          --n;
        }
        // Remove the actual header
        arr += 2;
        n -= 2;

        // DEBUG
        // print_readings(arr, n);

        // Filter out the tail
        for (int i = n - 1; i >= 0; --i) {
          if (arr[i].usec > 30000) {
            n = arr[i].v ? i : i - 1;
            break;
          }
        }

        // DEBUG
        // print_readings(arr, n);

        unsigned x = convert(arr,n);
        output("converted to %x\n", x);
        const char *key = key_to_str(x);
        if(key)
            printk("%s\n", key);
        else
            // failed: dump out the data so we can figure out what happened.
            print_readings(arr,n);
    }
	printk("stopping ir send/rec!\n");
    clean_reboot();
}
