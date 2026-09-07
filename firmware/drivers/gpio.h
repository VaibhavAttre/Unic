#ifndef UNIC_GPIO_H
#define UNIC_GPIO_H

#include "stm32h5xx.h"

void gpio_configure_usart3_pins(void);

/*
 * Configure the 9 RMII/MDIO signals (see part2plan.md #2) for AF11 (confirmed
 * against the Nucleo-H563ZI's shipped Zephyr board files -- both the pin set
 * and the AF11 assignment match this project's table exactly):
 *   PA1 ETH_REF_CLK, PA2 ETH_MDIO, PA7 ETH_CRS_DV, PB15 ETH_TXD1,
 *   PC1 ETH_MDC, PC4 ETH_RXD0, PC5 ETH_RXD1, PG11 ETH_TX_EN, PG13 ETH_TXD0.
 *
 * Does not touch RCC (caller must enable GPIOA/B/C/G first). Must run before
 * eth_mac_init(): the MAC's DMA software reset will not complete unless
 * ETH_REF_CLK is already muxed to AF11 and receiving the PHY's clock. This
 * also hands PC1/PA2 to the MAC's hardware MDIO block, so it must not run
 * before eth_mdio.c's bit-banged transfer is retired (it has been, as of the
 * hardware-MDIO swap -- see eth_mdio.c).
 */
void gpio_configure_rmii_pins(void);

#endif
