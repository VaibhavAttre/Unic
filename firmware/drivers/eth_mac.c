#include "eth_mac.h"

void eth_mac_init(const uint8_t mac_addr[6])
{
    /* SBS_PMCR_ETH_SEL_PHY = 0b100 selects RMII (confirmed against RM0481,
     * covering the H562/H563/H523/H533/H573 line -- this field is a 3-bit
     * selector on H5, unlike the single MII/RMII toggle bit on H7). The
     * dummy read-back matches ST's own documented workaround for the
     * DMAMR.SWR-stuck erratum: without it, some parts sample a stale PMCR
     * value on the immediately-following DMA reset. */
    SBS->PMCR = (SBS->PMCR & ~SBS_PMCR_ETH_SEL_PHY) | SBS_PMCR_ETH_SEL_PHY_2;
    (void)SBS->PMCR;

    /* Reset the MAC/MTL/DMA subsystem into a known state. This blocks
     * (no timeout) like the RCC ready-waits elsewhere in this project:
     * unlike PHY auto-negotiation, completion here depends only on the
     * MCU's own clocking, which the caller is required to have already
     * set up correctly (see the ordering requirements in eth_mac.h). */
    ETH->DMAMR |= ETH_DMAMR_SWR;
    while ((ETH->DMAMR & ETH_DMAMR_SWR) != 0U)
    {
    }

    /* MACA0LR holds address bytes 0-3, MACA0HR holds bytes 4-5 plus AE
     * (Address Enable, required for this entry to be used in RX filtering). */
    ETH->MACA0HR = ETH_MACA0HR_AE
                 | ((uint32_t)mac_addr[5] << 8)
                 | (uint32_t)mac_addr[4];
    ETH->MACA0LR = ((uint32_t)mac_addr[3] << 24)
                 | ((uint32_t)mac_addr[2] << 16)
                 | ((uint32_t)mac_addr[1] << 8)
                 | (uint32_t)mac_addr[0];
}
