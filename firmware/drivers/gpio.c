#include "gpio.h"

#define USART3_TX_PIN 8U
#define USART3_RX_PIN 9U
#define GPIO_AF7_USART3 0x7U
#define GPIO_MODER_AF 0x2U

void gpio_configure_usart3_pins(void)
{
    /* PD8 = USART3_TX, PD9 = USART3_RX, both alternate function AF7. */
    GPIOD->MODER &= ~((0x3U << (USART3_TX_PIN * 2U)) | (0x3U << (USART3_RX_PIN * 2U)));
    GPIOD->MODER |= (GPIO_MODER_AF << (USART3_TX_PIN * 2U)) | (GPIO_MODER_AF << (USART3_RX_PIN * 2U));

    GPIOD->AFR[1] &= ~((0xFU << ((USART3_TX_PIN - 8U) * 4U)) | (0xFU << ((USART3_RX_PIN - 8U) * 4U)));
    GPIOD->AFR[1] |= (GPIO_AF7_USART3 << ((USART3_TX_PIN - 8U) * 4U)) | (GPIO_AF7_USART3 << ((USART3_RX_PIN - 8U) * 4U));
}
