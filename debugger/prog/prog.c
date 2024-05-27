#include "rpi.h"
#include "jtag.h"
#include "cp14-debug.h"

enum {
  LED_PIN = 12
};

void notmain(void) {
  gpio_set_output(LED_PIN);
  gpio_set_function(JTAG_RTCK, GPIO_FUNC_ALT4);
  gpio_set_function(JTAG_TDO, GPIO_FUNC_ALT4);
  gpio_set_function(JTAG_TCK, GPIO_FUNC_ALT4);
  gpio_set_function(JTAG_TDI, GPIO_FUNC_ALT4);
  gpio_set_function(JTAG_TMS, GPIO_FUNC_ALT4);

  output("Hello world!\n");
  int v = 0;
  while (1) {
    gpio_write(LED_PIN, v);
    v = !v;
    delay_ms(500);
  }
}
