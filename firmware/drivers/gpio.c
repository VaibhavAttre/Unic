#include "gpio.h"

#define USART3_TX_PIN 8U
#define USART3_RX_PIN 9U
#define GPIO_AF7_USART3 0x7U
#define GPIO_MODER_AF 0x2U
#define GPIO_OSPEEDR_VERY_HIGH 0x3U
#define GPIO_AF11_ETH 0xBU

void gpio_configure_usart3_pins(void)
{
    /* PD8 = USART3_TX, PD9 = USART3_RX, both alternate function AF7. */
    GPIOD->MODER &= ~((0x3U << (USART3_TX_PIN * 2U)) | (0x3U << (USART3_RX_PIN * 2U)));
    GPIOD->MODER |= (GPIO_MODER_AF << (USART3_TX_PIN * 2U)) | (GPIO_MODER_AF << (USART3_RX_PIN * 2U));

    GPIOD->AFR[1] &= ~((0xFU << ((USART3_TX_PIN - 8U) * 4U)) | (0xFU << ((USART3_RX_PIN - 8U) * 4U)));
    GPIOD->AFR[1] |= (GPIO_AF7_USART3 << ((USART3_TX_PIN - 8U) * 4U)) | (GPIO_AF7_USART3 << ((USART3_RX_PIN - 8U) * 4U));
}

/* Set one pin to alternate-function mode with the given AF number and
 * very-high output speed (needed for the 50 MHz RMII signals; harmless on
 * the RMII inputs too). */
static void gpio_set_af_pin(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    uint32_t afr_idx = pin / 8U;
    uint32_t afr_shift = (pin % 8U) * 4U;

    port->MODER &= ~(0x3U << (pin * 2U));
    port->MODER |= (GPIO_MODER_AF << (pin * 2U));

    port->OSPEEDR &= ~(0x3U << (pin * 2U));
    port->OSPEEDR |= (GPIO_OSPEEDR_VERY_HIGH << (pin * 2U));

    port->AFR[afr_idx] &= ~(0xFU << afr_shift);
    port->AFR[afr_idx] |= (af << afr_shift);
}

void gpio_configure_rmii_pins(void)
{
    gpio_set_af_pin(GPIOA, 1U, GPIO_AF11_ETH);  /* ETH_REF_CLK */
    gpio_set_af_pin(GPIOA, 2U, GPIO_AF11_ETH);  /* ETH_MDIO    */
    gpio_set_af_pin(GPIOA, 7U, GPIO_AF11_ETH);  /* ETH_CRS_DV  */
    gpio_set_af_pin(GPIOB, 15U, GPIO_AF11_ETH); /* ETH_TXD1    */
    gpio_set_af_pin(GPIOC, 1U, GPIO_AF11_ETH);  /* ETH_MDC     */
    gpio_set_af_pin(GPIOC, 4U, GPIO_AF11_ETH);  /* ETH_RXD0    */
    gpio_set_af_pin(GPIOC, 5U, GPIO_AF11_ETH);  /* ETH_RXD1    */
    gpio_set_af_pin(GPIOG, 11U, GPIO_AF11_ETH); /* ETH_TX_EN   */
    gpio_set_af_pin(GPIOG, 13U, GPIO_AF11_ETH); /* ETH_TXD0    */
}
