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
