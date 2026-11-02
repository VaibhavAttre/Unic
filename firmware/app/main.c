#include "stm32h5xx.h"

#include "rcc.h"
#include "gpio.h"
#include "usart.h"
#include "eth_mdio.h"
#include "eth_mac.h"

#define UNIC_FW_VERSION "0.1.0"
#define UNIC_USART3_BAUD 115200U

/* LAN8742 station address on the Nucleo-H563ZI (RMII address straps = 0). */
#define UNIC_PHY_ADDR 0U

/* Locally-administered unicast MAC address (U/L bit set, multicast bit
 * clear in the first octet): no vendor OUI is assigned to this project, so
 * this avoids colliding with a real one on the wire. */
static const uint8_t UNIC_MAC_ADDR[6] = { 0x02U, 0x55U, 0x4EU, 0x49U, 0x43U, 0x01U };

/* PHY poll cadence for link-state changes. */
#define UNIC_LINK_POLL_MS 200U

/* Number of BSR reads taken per poll; the link bit is the majority vote so a
 * single corrupted MDIO read cannot fake a transition. */
#define UNIC_LINK_SAMPLES 5U

/* Upper bound on the wait for auto-negotiation to converge after link-up. A
 * healthy link resolves in well under a second; the cap stops a PHY that never
 * completes from stalling link-state polling. */
#define UNIC_AUTONEG_TIMEOUT_MS 3000U

/* Report the cause of the most recent reset over UART, then clear the flags so
 * the next boot reports only its own cause. */
static void report_reset_cause(void)
{
    uint32_t rsr = RCC->RSR;

    usart3_write_str("reset:");
    if (rsr & RCC_RSR_LPWRRSTF) { usart3_write_str(" lpwr"); }
    if (rsr & RCC_RSR_WWDGRSTF) { usart3_write_str(" wwdg"); }
    if (rsr & RCC_RSR_IWDGRSTF) { usart3_write_str(" iwdg"); }
    if (rsr & RCC_RSR_SFTRSTF)  { usart3_write_str(" soft"); }
    if (rsr & RCC_RSR_BORRSTF)  { usart3_write_str(" bor"); }
    if (rsr & RCC_RSR_PINRSTF)  { usart3_write_str(" pin"); }
    usart3_write_str("\n");

    RCC->RSR |= RCC_RSR_RMVF;
}

/* Blocking millisecond delay built on SysTick as a free-running down-counter.
 * SysTick runs off the processor clock (RCC_SYSCLK_HZ, 250 MHz after the PLL
 * switch), so a reload of RCC_SYSCLK_HZ/1000 ticks spans 1 ms. */
static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = (RCC_SYSCLK_HZ / 1000U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (ms-- > 0U)
    {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U)
        {
        }
    }

    SysTick->CTRL = 0U;
}

/* Wait (bounded) for auto-negotiation to finish after a link-up, then report the
 * resolved speed and duplex from the LAN8742 status register. This is the datum
 * the Ethernet MAC needs to configure the RMII path once it is brought up. */
static void report_autoneg_result(void)
{
    uint32_t waited_ms = 0U;

    while ((eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_BSR) & PHY_BSR_AUTONEG_DONE) == 0U)
    {
        if (waited_ms >= UNIC_AUTONEG_TIMEOUT_MS)
        {
            usart3_write_str("autoneg: timeout\n");
            return;
        }
        delay_ms(50U);
        waited_ms += 50U;
    }

    uint16_t scsr = eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_PSCSR);
    const char *mode;

    switch (scsr & PHY_PSCSR_SPEED_MASK)
    {
    case PHY_PSCSR_SPEED_10_HD:  mode = "10/half";  break;
    case PHY_PSCSR_SPEED_10_FD:  mode = "10/full";  break;
    case PHY_PSCSR_SPEED_100_HD: mode = "100/half"; break;
    case PHY_PSCSR_SPEED_100_FD: mode = "100/full"; break;
    default:                     mode = "unknown";  break;
    }

    usart3_write_str("autoneg: done ");
    usart3_write_str(mode);
    usart3_write_str(" lpa=");
    usart3_write_hex16(eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_ANLPAR));
    usart3_write_str("\n");
}

int main(void)
{
    rcc_init_hsi64();
    rcc_init_pll_250mhz();
    rcc_enable_gpioa();
    rcc_enable_gpiob();
    rcc_enable_gpioc();
    rcc_enable_gpiod();
    rcc_enable_gpiog();
    rcc_enable_usart3();
    rcc_enable_sbs();
    rcc_enable_eth();

    gpio_configure_usart3_pins();
    /* Must run before eth_mac_init(): its DMA reset needs ETH_REF_CLK
     * already muxed in and receiving the PHY's clock. This also hands
     * PC1/PA2 to the MAC's hardware MDIO block, which eth_mdio.c now uses. */
    gpio_configure_rmii_pins();
    usart3_init(RCC_PCLK1_HZ, UNIC_USART3_BAUD);

    usart3_write_str("uNIC fw v" UNIC_FW_VERSION " | PLL 250MHz (HSI/16 x125 /2) | "
                     __DATE__ " " __TIME__ "\n");
    report_reset_cause();

    eth_mac_init(UNIC_MAC_ADDR);
    eth_mdio_init();

    /* Identify the PHY once so a broken MDIO path is obvious in the log:
     * a healthy LAN8742 reports id1=0x0007, id2 0xc13x (low nibble = silicon revision). */
    usart3_write_str("PHY id1=");
    usart3_write_hex16(eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_ID1));
    usart3_write_str(" id2=");
    usart3_write_hex16(eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_ID2));
    usart3_write_str(" bsr=");
    usart3_write_hex16(eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_BSR));
    usart3_write_str("\n");

    int prev_link = -1;

    for (;;)
    {
        uint32_t up_votes = 0U;
        for (uint32_t i = 0U; i < UNIC_LINK_SAMPLES; ++i)
        {
            if (eth_mdio_read(UNIC_PHY_ADDR, PHY_REG_BSR) & PHY_BSR_LINK_UP)
            {
                ++up_votes;
            }
        }

        int link = (up_votes * 2U > UNIC_LINK_SAMPLES) ? 1 : 0;

        if (link != prev_link)
        {
            usart3_write_str(link ? "link up\n" : "link down\n");
            if (link)
            {
                report_autoneg_result();
            }
            prev_link = link;
        }

        delay_ms(UNIC_LINK_POLL_MS);
    }
}
