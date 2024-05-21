// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define TX 14
#define RX 15
#define AUXENB          0x20215004
#define AUX_MU_IO_REG   0x20215040
#define AUX_MU_IER_REG  0x20215044
#define AUX_MU_IIR_REG  0x20215048
#define AUX_MU_LCR_REG  0x2021504C
#define AUX_MU_LSR_REG  0x20215054
#define AUX_MU_CNTL_REG 0x20215060
#define AUX_MU_BAUD     0x20215068
#define UART_ENABLE       0b1
#define UART_TX_ENABLE    0b10
#define UART_RX_ENABLE    0b1
#define UART_TX_INTERRUPT 0b1
#define UART_RX_INTERRUPT 0b10
#define UART_TX_EMPTY     0b1000000
#define UART_TX_READY     0b100000
#define UART_RX_READY     0b1
#define UART_8_BIT_MODE   0b11
// BCM2835-ARM-Peripherals.annot.PDF, p.11, baud rate equation
//   baudrate = (system clock frequency (250 MHz)) / (8 * (baudrate reg + 1))
// Therefore
//   baudrate reg = 25e7 / (8 * baudrate) - 1
// #define UART_BAUD_REG_VAL 270 // 115200
// #define UART_BAUD_REG_VAL 134 // 230400
#define UART_BAUD_REG_VAL 33 // 921600 
#define UART_CLR_TX       0b100
#define UART_CLR_RX       0b10

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void hw_uart_init(unsigned reg_val) {
  // Setup GPIO
  dev_barrier();
  gpio_set_function(TX, GPIO_FUNC_ALT5);
  gpio_set_function(RX, GPIO_FUNC_ALT5);
  gpio_write(TX, 1);

  // Turn mini UART on
  dev_barrier();
  uint32_t flags = GET32(AUXENB);
  flags |= UART_ENABLE;
  PUT32(AUXENB, flags);

  // Disable mini UART
  dev_barrier();
  PUT32(AUX_MU_CNTL_REG, 0);

  // Disable interrupts
  PUT32(AUX_MU_IER_REG, 0);

  // Set 8n1
  PUT32(AUX_MU_LCR_REG, UART_8_BIT_MODE);

  // Set baudrate
  PUT32(AUX_MU_BAUD, reg_val);

  // Clear FIFO's
  PUT32(AUX_MU_IIR_REG, UART_CLR_TX | UART_CLR_RX);

  // Enable mini UART
  PUT32(AUX_MU_CNTL_REG, UART_TX_ENABLE | UART_RX_ENABLE);

  dev_barrier();

  // DEBUG
  output("UART: initialization done!\n");
}

// disable the uart.
void hw_uart_disable(void) {
  dev_barrier();
  uart_flush_tx();
  dev_barrier();

  uint32_t flags = GET32(AUXENB);
  flags &= ~UART_ENABLE;
  PUT32(AUXENB, flags);

  dev_barrier();
}


#if 0
// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
  // DEBUG
  dev_barrier();

  while (!uart_has_data());
  int ret = GET32(AUX_MU_IO_REG) & 0xff;

  // DEBUG
  dev_barrier();
  return ret;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
  // DEBUG
  dev_barrier();

  int ret = (GET32(AUX_MU_LSR_REG) & UART_TX_READY) ? 1 : 0;

  // DEBUG
  dev_barrier();
  return ret;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
  // DEBUG
  dev_barrier();

  while (!uart_can_put8());
  PUT32(AUX_MU_IO_REG, c);

  // DEBUG
  dev_barrier();
  return 0;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
  // DEBUG
  dev_barrier();

  int ret = (GET32(AUX_MU_LSR_REG) & UART_RX_READY) ? 1 : 0;

  // DEBUG
  dev_barrier();
  return ret;
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
  // DEBUG
  dev_barrier();

  if(!uart_has_data()) {
    dev_barrier();
    return -1;
  }

  int ret = uart_get8();

  // DEBUG
  dev_barrier();
  return ret;
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
  // DEBUG
  dev_barrier();

  int ret = (GET32(AUX_MU_LSR_REG) & UART_TX_EMPTY) ? 1 : 0;

  // DEBUG
  dev_barrier();
  return ret;
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
  // DEBUG
  dev_barrier();

  while(!uart_tx_is_empty());

  // DEBUG
  dev_barrier();
}
#endif
