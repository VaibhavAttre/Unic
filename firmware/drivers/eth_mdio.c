#include "eth_mdio.h"

#include "stm32h5xx.h"

/*
 * CSR clock range: MACMDIOAR_CR selects the MDC divider from HCLK (the CSR
 * clock here, since ETH has no separate kernel clock mux on this part).
 * At HCLK = 250 MHz (see rcc_init_pll_250mhz()) this sits in the documented
 * 250-300 MHz bracket, DIV124 -> MDC = 250 MHz / 124 ~= 2.02 MHz, under the
 * LAN8742's 2.5 MHz IEEE limit. Revisit if the core clock changes again.
 */
#define MDIO_CSR_CLOCK_RANGE ETH_MACMDIOAR_CR_DIV124

static void mdio_wait_not_busy(void)
{
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) != 0U)
    {
    }
}

void eth_mdio_init(void)
{
}

uint16_t eth_mdio_read(uint8_t phy_addr, uint8_t reg_addr)
{
    mdio_wait_not_busy();

    ETH->MACMDIOAR = ((uint32_t)(phy_addr & 0x1FU) << ETH_MACMDIOAR_PA_Pos)
                    | ((uint32_t)(reg_addr & 0x1FU) << ETH_MACMDIOAR_RDA_Pos)
                    | MDIO_CSR_CLOCK_RANGE
                    | ETH_MACMDIOAR_MOC_RD
                    | ETH_MACMDIOAR_MB;

    mdio_wait_not_busy();
    return (uint16_t)(ETH->MACMDIODR & ETH_MACMDIODR_MD);
}

void eth_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    mdio_wait_not_busy();

    ETH->MACMDIODR = (uint32_t)value & ETH_MACMDIODR_MD;
    ETH->MACMDIOAR = ((uint32_t)(phy_addr & 0x1FU) << ETH_MACMDIOAR_PA_Pos)
                    | ((uint32_t)(reg_addr & 0x1FU) << ETH_MACMDIOAR_RDA_Pos)
                    | MDIO_CSR_CLOCK_RANGE
                    | ETH_MACMDIOAR_MOC_WR
                    | ETH_MACMDIOAR_MB;

    mdio_wait_not_busy();
}
