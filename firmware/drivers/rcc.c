#include "rcc.h"

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
