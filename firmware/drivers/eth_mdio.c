#include "eth_mdio.h"

#include "stm32h5xx.h"

#define MDC_PIN   1U   /* PC1 */
#define MDIO_PIN  2U   /* PA2 */

/*
 * Half-period spin count for the MDC bit clock. The LAN8742 accepts MDC up to
 * 2.5 MHz; this loop only needs to stay below that bound, not hit an exact
 * frequency, so the count is deliberately approximate for a 64 MHz core.
 */
#define MDIO_HALF_PERIOD_SPINS 16U

static void mdio_delay(void)
{
    for (volatile uint32_t i = 0U; i < MDIO_HALF_PERIOD_SPINS; ++i)
    {
    }
}

static void mdc_high(void)
{
    GPIOC->BSRR = (1U << MDC_PIN);
}

static void mdc_low(void)
{
    GPIOC->BSRR = (1U << (MDC_PIN + 16U));
}

/*
 * MDIO is an open-drain line with a pull-up: writing 1 releases it (the pull-up
 * or the PHY then sets the level), writing 0 drives it low. The pin therefore
 * both sources and samples data without ever changing direction.
 */
static void mdio_release(void)
{
    GPIOA->BSRR = (1U << MDIO_PIN);
}

static void mdio_drive(uint32_t bit)
{
    GPIOA->BSRR = bit ? (1U << MDIO_PIN) : (1U << (MDIO_PIN + 16U));
}

static uint32_t mdio_sample(void)
{
    return (GPIOA->IDR >> MDIO_PIN) & 1U;
}

/* Drive one bit to the PHY: data is set up while MDC is low and latched by the
 * PHY on the rising edge. MDC returns low so it idles low between bits. */
static void mdio_clock_out(uint32_t bit)
{
    mdio_drive(bit);
    mdio_delay();
    mdc_high();
    mdio_delay();
    mdc_low();
}

/* Clock one bit in from the PHY: the PHY updates MDIO shortly after the rising
 * edge; it is sampled while MDC is high and before the falling edge. */
static uint32_t mdio_clock_in(void)
{
    uint32_t bit;

    mdc_high();
    mdio_delay();
    bit = mdio_sample();
    mdc_low();
    mdio_delay();
    return bit;
}

static void mdio_write_field(uint32_t value, uint32_t bits)
{
    while (bits-- > 0U)
    {
        mdio_clock_out((value >> bits) & 1U);
    }
}

/* 32 clocks with MDIO released resynchronises the PHY management state machine
 * ahead of every frame. */
static void mdio_preamble(void)
{
    mdio_release();
    for (uint32_t i = 0U; i < 32U; ++i)
    {
        mdio_delay();
        mdc_high();
        mdio_delay();
        mdc_low();
    }
}

void eth_mdio_init(void)
{
    /* PC1 (MDC): push-pull output. */
    GPIOC->MODER &= ~(0x3U << (MDC_PIN * 2U));
    GPIOC->MODER |= (0x1U << (MDC_PIN * 2U));
    GPIOC->OTYPER &= ~(1U << MDC_PIN);

    /* PA2 (MDIO): open-drain output with pull-up. */
    GPIOA->MODER &= ~(0x3U << (MDIO_PIN * 2U));
    GPIOA->MODER |= (0x1U << (MDIO_PIN * 2U));
    GPIOA->OTYPER |= (1U << MDIO_PIN);
    GPIOA->PUPDR &= ~(0x3U << (MDIO_PIN * 2U));
    GPIOA->PUPDR |= (0x1U << (MDIO_PIN * 2U));

    mdc_low();
    mdio_release();
}

uint16_t eth_mdio_read(uint8_t phy_addr, uint8_t reg_addr)
{
    uint16_t value = 0U;

    mdio_preamble();
    mdio_write_field(0x1U, 2U);              /* ST = 01           */
    mdio_write_field(0x2U, 2U);              /* OP = 10 (read)    */
    mdio_write_field(phy_addr & 0x1FU, 5U);  /* PHYAD             */
    mdio_write_field(reg_addr & 0x1FU, 5U);  /* REGAD             */

    /* Turnaround: master releases MDIO for two clocks; the PHY pulls it low on
     * the second before it starts shifting out data. */
    mdio_release();
    (void)mdio_clock_in();
    (void)mdio_clock_in();

    for (uint32_t i = 0U; i < 16U; ++i)
    {
        value = (uint16_t)((value << 1) | mdio_clock_in());
    }

    mdio_release();
    return value;
}

void eth_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    mdio_preamble();
    mdio_write_field(0x1U, 2U);              /* ST = 01           */
    mdio_write_field(0x1U, 2U);              /* OP = 01 (write)   */
    mdio_write_field(phy_addr & 0x1FU, 5U);  /* PHYAD             */
    mdio_write_field(reg_addr & 0x1FU, 5U);  /* REGAD             */
    mdio_write_field(0x2U, 2U);              /* TA = 10 (driven)  */
    mdio_write_field(value, 16U);            /* DATA              */

    mdio_release();
}
