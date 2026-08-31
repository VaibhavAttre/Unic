#include "usart.h"

#include "stm32h5xx.h"

static void usart3_write_byte(uint8_t b)
{
    while ((USART3->ISR & USART_ISR_TXE) == 0U)
    {
    }
    USART3->TDR = b;
}

void usart3_init(uint32_t kernel_clk_hz, uint32_t baud)
{
    /* Route the USART3 kernel clock to PCLK1 (USART3SEL = 0b000). */
    RCC->CCIPR1 &= ~RCC_CCIPR1_USART3SEL;

    /* Disable the peripheral while it is reconfigured. CR1/CR2/CR3 reset state
     * already gives 8 data bits, no parity, 1 stop bit and oversampling by 16. */
    USART3->CR1 = 0U;
    USART3->CR2 = 0U;
    USART3->CR3 = 0U;

    /* Oversampling by 16: BRR = round(f_kernel / baud). */
    USART3->BRR = (kernel_clk_hz + baud / 2U) / baud;

    USART3->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART3->CR1 |= USART_CR1_UE;

    /* Wait for the transmitter to acknowledge enable before the first write. */
    while ((USART3->ISR & USART_ISR_TEACK) == 0U)
    {
    }
}

void usart3_write_str(const char *s)
{
    for (; *s != '\0'; ++s)
    {
        if (*s == '\n')
        {
            usart3_write_byte((uint8_t)'\r');
        }
        usart3_write_byte((uint8_t)*s);
    }

    while ((USART3->ISR & USART_ISR_TC) == 0U)
    {
    }
}
