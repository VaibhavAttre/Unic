#include "rcc.h"

void rcc_init_hsi64(void)
{
    /* Ensure HSI is on and ready before it is used as the system clock. */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    /* HSIDIV = /1: hsi_ck = 64 MHz. HSIDIVF confirms the divider has taken effect. */
    RCC->CR &= ~RCC_CR_HSIDIV;
    while ((RCC->CR & RCC_CR_HSIDIVF) == 0U)
    {
    }

    /* Select HSI as the system clock source (SW = 0b00) and wait for the switch. */
    RCC->CFGR1 &= ~RCC_CFGR1_SW;
    while ((RCC->CFGR1 & RCC_CFGR1_SWS) != 0U)
    {
    }

    /* AHB and APB1 prescalers = /1: HCLK = PCLK1 = 64 MHz. */
    RCC->CFGR2 &= ~(RCC_CFGR2_HPRE | RCC_CFGR2_PPRE1);
}

void rcc_enable_gpioa(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
}

void rcc_enable_gpioc(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;
}

void rcc_enable_gpiod(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    (void)RCC->AHB2ENR;
}

void rcc_enable_usart3(void)
{
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    (void)RCC->APB1LENR;
}

void rcc_init_pll_250mhz(void)
{
    /* VOS0 is required above 190 MHz (the flash-latency table below is keyed
     * off it); wait for the regulator to actually reach it before touching
     * the clock. PWR has no bus-clock gate on this line, unlike GPIO/USART. */
    PWR->VOSCR |= (PWR_VOSCR_VOS_0 | PWR_VOSCR_VOS_1);
    while ((PWR->VOSSR & PWR_VOSSR_VOSRDY) == 0U)
    {
    }

    /* Flash must be able to keep up with 250 MHz before SYSCLK moves there;
     * raising latency ahead of the switch is safe at the current (slower)
     * HSI clock. 5 wait-states covers 190-250 MHz at VOS0. */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_5WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS)
    {
    }

    /* PLL1 must be off while its dividers/source are (re)configured. */
    RCC->CR &= ~RCC_CR_PLL1ON;
    while ((RCC->CR & RCC_CR_PLL1RDY) != 0U)
    {
    }

    /* HSI (64 MHz) / M=16 = 4 MHz reference, x N=125 = 500 MHz VCO, / P=2 =
     * 250 MHz SYSCLK. Q/R are left at a valid divisor but unused: only the
     * P output is enabled below.
     *
     * PLL1SRC_0 alone selects HSI (both bits would be HSE). PLL1RGE_0 alone
     * selects the 2-4 MHz input range for a 4 MHz reference. PLL1VCOSEL is
     * left clear for the wide VCO range (192-836 MHz), which covers the
     * 500 MHz target -- the medium range (150-420 MHz) would not. */
    RCC->PLL1CFGR = RCC_PLL1CFGR_PLL1SRC_0
                  | (16U << RCC_PLL1CFGR_PLL1M_Pos)
                  | RCC_PLL1CFGR_PLL1RGE_0;
    RCC->PLL1DIVR = ((125U - 1U) << RCC_PLL1DIVR_PLL1N_Pos)
                   | ((2U - 1U) << RCC_PLL1DIVR_PLL1P_Pos)
                   | ((2U - 1U) << RCC_PLL1DIVR_PLL1Q_Pos)
                   | ((2U - 1U) << RCC_PLL1DIVR_PLL1R_Pos);
    RCC->PLL1CFGR |= RCC_PLL1CFGR_PLL1PEN;

    RCC->CR |= RCC_CR_PLL1ON;
    while ((RCC->CR & RCC_CR_PLL1RDY) == 0U)
    {
    }

    /* AHB and APB1 prescalers stay /1: PCLK1 max is also 250 MHz on this
     * part, so HCLK = PCLK1 = SYSCLK. */
    RCC->CFGR2 &= ~(RCC_CFGR2_HPRE | RCC_CFGR2_PPRE1);

    /* Select PLL1 as the system clock source (SW = 0b11) and wait for the
     * switch to complete (SWS reads back 0b11). SW and SWS are separate
     * fields at different bit offsets, not aliases of each other. */
    RCC->CFGR1 = (RCC->CFGR1 & ~RCC_CFGR1_SW) | (RCC_CFGR1_SW_0 | RCC_CFGR1_SW_1);
    while ((RCC->CFGR1 & RCC_CFGR1_SWS) != (RCC_CFGR1_SWS_0 | RCC_CFGR1_SWS_1))
    {
    }
}
