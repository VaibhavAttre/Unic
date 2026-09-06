#include "stm32h5xx.h"

#include "rcc.h"
#include "gpio.h"
#include "usart.h"
#include "eth_mdio.h"

#define UNIC_FW_VERSION "0.1.0"
#define UNIC_USART3_BAUD 115200U

/* LAN8742 station address on the Nucleo-H563ZI (RMII address straps = 0). */
#define UNIC_PHY_ADDR 0U

/* PHY poll cadence for link-state changes. */
#define UNIC_LINK_POLL_MS 200U

/* Number of BSR reads taken per poll; the link bit is the majority vote so a
 * single corrupted MDIO read cannot fake a transition. */
#define UNIC_LINK_SAMPLES 5U

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
 * The core runs at 64 MHz (HSI, all prescalers /1), so a reload of 64000 ticks
 * spans 1 ms. */
static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = (RCC_HSI_HZ / 1000U) - 1U;
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

int main(void)
{
    rcc_init_hsi64();
    rcc_enable_gpioa();
    rcc_enable_gpioc();
    rcc_enable_gpiod();
    rcc_enable_usart3();

    gpio_configure_usart3_pins();
    usart3_init(RCC_PCLK1_HZ, UNIC_USART3_BAUD);

    usart3_write_str("uNIC fw v" UNIC_FW_VERSION " | HSI 64MHz | "
                     __DATE__ " " __TIME__ "\n");
    report_reset_cause();

    eth_mdio_init();

    /* Identify the PHY once so a broken MDIO path is obvious in the log:
     * a healthy LAN8742 reports id1=0x0007, id2 upper bits 0xc0f0. */
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
            prev_link = link;
        }

        delay_ms(UNIC_LINK_POLL_MS);
    }
}
