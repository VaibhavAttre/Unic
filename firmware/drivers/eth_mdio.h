#ifndef UNIC_ETH_MDIO_H
#define UNIC_ETH_MDIO_H

#include <stdint.h>

/*
 * Bit-banged IEEE 802.3 Clause 22 station management (MDIO/MDC) master.
 * MDC is driven on PC1, MDIO on PA2 (open-drain with pull-up), matching the
 * LAN8742 wiring on the Nucleo-H563ZI. The Ethernet MAC peripheral and the
 * RMII data path are not involved, so PHY registers are reachable without any
 * MAC clocking or RMII reference-clock setup.
 */

/* Clause 22 register addresses used for link detection. */
#define PHY_REG_BCR   0x00U   /* Basic Control Register  */
#define PHY_REG_BSR   0x01U   /* Basic Status Register   */
#define PHY_REG_ID1   0x02U   /* PHY Identifier 1        */
#define PHY_REG_ID2   0x03U   /* PHY Identifier 2        */

/* Basic Status Register bits. LINK_UP latches low on link loss and reads the
 * live state on the following read. */
#define PHY_BSR_LINK_UP        (1U << 2)
#define PHY_BSR_AUTONEG_DONE   (1U << 5)

/* Configure PC1/PA2 for bit-banged MDIO. The GPIOA and GPIOC bus clocks must
 * already be enabled. */
void eth_mdio_init(void);

/* Read a 16-bit PHY register. */
uint16_t eth_mdio_read(uint8_t phy_addr, uint8_t reg_addr);

/* Write a 16-bit PHY register. */
void eth_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);

#endif
