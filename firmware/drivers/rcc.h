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

/*
 * PLL1 configuration (rcc_init_pll_250mhz): HSI 64 MHz / M=16 = 4 MHz PLL1
 * input (VCIRANGE_1, 2-4 MHz), x N=125 = 500 MHz VCO (VCORANGE_WIDE,
 * 192-836 MHz), / P=2 = 250 MHz SYSCLK -- the part's documented maximum.
 * HPRE/PPRE1 stay at /1 (PCLK1 max is also 250 MHz on this line), so
 * HCLK = PCLK1 = 250 MHz too.
 */
#define RCC_SYSCLK_HZ 250000000U
#define RCC_PCLK1_HZ  250000000U

/* Select HSI as system clock and force HSIDIV, HPRE and PPRE1 to /1. */
void rcc_init_hsi64(void);

/*
 * Raise the core to 250 MHz via PLL1 fed from HSI (see the PLL1 configuration
 * comment above). Must run after rcc_init_hsi64(), since PLL1 is sourced from
 * HSI and the sequence assumes SYSCLK is still HSI on entry. Ramps the PWR
 * voltage scale to VOS0 and sets 5 flash wait-states before switching SYSCLK,
 * per the reference manual's increase-frequency ordering.
 */
void rcc_init_pll_250mhz(void);

/* Enable the peripheral bus clock for GPIO port A. */
void rcc_enable_gpioa(void);

/* Enable the peripheral bus clock for GPIO port C. */
void rcc_enable_gpioc(void);

/* Enable the peripheral bus clock for GPIO port D. */
void rcc_enable_gpiod(void);

/* Enable the peripheral bus clock for USART3. */
void rcc_enable_usart3(void);

#endif
