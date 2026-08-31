#include "stm32h5xx.h"

#include "rcc.h"
#include "gpio.h"
#include "usart.h"

#define UNIC_FW_VERSION "0.1.0"
#define UNIC_USART3_BAUD 115200U

int main(void)
{
    rcc_init_hsi64();
    rcc_enable_gpiod();
    rcc_enable_usart3();

    gpio_configure_usart3_pins();
    usart3_init(RCC_PCLK1_HZ, UNIC_USART3_BAUD);

    usart3_write_str("uNIC fw v" UNIC_FW_VERSION " | HSI 64MHz | "
                     __DATE__ " " __TIME__ "\n");

    for (;;)
    {
    }
}
