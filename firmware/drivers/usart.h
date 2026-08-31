#ifndef UNIC_USART_H
#define UNIC_USART_H

#include <stdint.h>

/*
 * Blocking, polled USART3 transmit driver.
 * The GPIO alternate-function pins and the USART3 bus clock must already be
 * configured before usart3_init() is called.
 */

/*
 * Configure USART3 for 8N1 at the given baud rate.
 * kernel_clk_hz is the USART3 kernel clock frequency (PCLK1 in this build);
 * it drives the oversampling-by-16 BRR divisor.
 */
void usart3_init(uint32_t kernel_clk_hz, uint32_t baud);

/* Transmit a NUL-terminated string, expanding '\n' to "\r\n". Blocks until
 * the last byte has left the shift register. */
void usart3_write_str(const char *s);

#endif
