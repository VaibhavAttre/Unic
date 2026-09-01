#ifndef UNIC_RCC_H
#define UNIC_RCC_H

#include "stm32h5xx.h"

/*
 * Clock configuration: SYSCLK sourced from HSI with no PLL.
 * HSI runs at 64 MHz; with HSIDIV and the AHB/APB1 prescalers all set to /1,
 * SYSCLK = HCLK = PCLK1 = 64 MHz. PCLK1 feeds the USART3 kernel clock and is
 * the reference for the BRR divisor.
 */
#define RCC_HSI_HZ    64000000U
#define RCC_PCLK1_HZ  64000000U

/* Select HSI as system clock and force HSIDIV, HPRE and PPRE1 to /1. */
void rcc_init_hsi64(void);

/* Enable the peripheral bus clock for GPIO port A. */
void rcc_enable_gpioa(void);

/* Enable the peripheral bus clock for GPIO port C. */
void rcc_enable_gpioc(void);

/* Enable the peripheral bus clock for GPIO port D. */
void rcc_enable_gpiod(void);

/* Enable the peripheral bus clock for USART3. */
void rcc_enable_usart3(void);

#endif
