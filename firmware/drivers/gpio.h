#ifndef UNIC_GPIO_H
#define UNIC_GPIO_H

#include "stm32h5xx.h"

void gpio_configure_usart3_pins(void);

/*
 * Configure the 8 RMII signals (see part2plan.md #2) for AF11 (confirmed
 * against the Nucleo-H563ZI's shipped Zephyr board files -- both the pin set
 * and the AF11 assignment match this project's table exactly):
 *   PA1 ETH_REF_CLK, PA2 ETH_MDIO, PA7 ETH_CRS_DV, PB15 ETH_TXD1,
 *   PC1 ETH_MDC, PC4 ETH_RXD0, PC5 ETH_RXD1, PG11 ETH_TX_EN, PG13 ETH_TXD0.
 *
 * Does not touch RCC (caller must enable GPIOA/B/C/G first) and is not yet
 * called from main(): eth_mdio_init() still bit-bangs PC1/PA2 as plain GPIO
 * for PHY link polling, and reconfiguring them to AF11 here would hand them
 * to the MAC's hardware MDIO block, which doesn't exist yet. Wire this in
 * once the hardware MDIO swap (part2plan.md task 3) replaces the bit-banged
 * transfer.
 */
void gpio_configure_rmii_pins(void);

#endif
