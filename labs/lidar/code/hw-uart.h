#ifndef HW_UART_H
#define HW_UART_H

#define UART_REG_VAL(baud) ((unsigned)25e7 / (8 * (baud)) - 1)

void hw_uart_init(unsigned baud_reg);
void hw_uart_disable(void);

#endif
