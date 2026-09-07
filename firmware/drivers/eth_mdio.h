#ifndef UNIC_ETH_MDIO_H
#define UNIC_ETH_MDIO_H

#include <stdint.h>

/*
 * IEEE 802.3 Clause 22 station management (MDIO/MDC) via the MAC's hardware
 * MACMDIOAR/MACMDIODR interface. MDC is driven on PC1, MDIO on PA2, muxed to
 * AF11 by gpio_configure_rmii_pins() -- these were bit-banged plain-GPIO in
 * Week 1; the swap to hardware MDIO removes the bit-bang timing margin behind
 * Week 1's occasional id2=0xc131 misread. The public API (eth_mdio_init/
 * eth_mdio_read/eth_mdio_write) is unchanged so main.c's link/auto-neg
 * polling and the PHY_REG_ and PHY_BSR_ constants below need no changes.
 *
 * Requires the ETH bus clock (rcc_enable_eth()) and RMII pin muxing
 * (gpio_configure_rmii_pins()) to already be done -- see eth_mdio_init().
 */

/* Clause 22 register addresses used for link detection. */
#define PHY_REG_BCR   0x00U   /* Basic Control Register  */
#define PHY_REG_BSR   0x01U   /* Basic Status Register   */
#define PHY_REG_ID1   0x02U   /* PHY Identifier 1        */
#define PHY_REG_ID2   0x03U   /* PHY Identifier 2        */
#define PHY_REG_ANAR   0x04U  /* Auto-Negotiation Advertisement        */
#define PHY_REG_ANLPAR 0x05U  /* Auto-Negotiation Link Partner Ability */

/* Basic Status Register bits. LINK_UP latches low on link loss and reads the
 * live state on the following read. */
#define PHY_BSR_LINK_UP        (1U << 2)
#define PHY_BSR_AUTONEG_DONE   (1U << 5)

/* LAN8742 vendor register 0x1F (PHY Special Control/Status). Once
 * auto-negotiation completes, bits 4:2 report the resolved speed and duplex. */
#define PHY_REG_PSCSR          0x1FU
#define PHY_PSCSR_SPEED_MASK   0x001CU
#define PHY_PSCSR_SPEED_10_HD  0x0004U
#define PHY_PSCSR_SPEED_10_FD  0x0014U
#define PHY_PSCSR_SPEED_100_HD 0x0008U
#define PHY_PSCSR_SPEED_100_FD 0x0018U

/* No register state to initialize: MACMDIOAR/MACMDIODR need no one-time
 * setup beyond the ETH bus clock and RMII pin muxing the caller has already
 * done. Kept as a no-op call so main.c's init sequence and this header's
 * public API do not need to change across the bit-bang -> hardware swap. */
void eth_mdio_init(void);

/* Read a 16-bit PHY register. */
uint16_t eth_mdio_read(uint8_t phy_addr, uint8_t reg_addr);

/* Write a 16-bit PHY register. */
void eth_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);

#endif
