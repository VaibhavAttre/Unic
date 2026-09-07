#ifndef UNIC_ETH_MAC_H
#define UNIC_ETH_MAC_H

#include "stm32h5xx.h"

/*
 * Bring up the Ethernet MAC/MTL/DMA subsystem for RMII operation:
 *   1. Select RMII on SBS_PMCR (must happen before any other MAC/DMA
 *      register access -- the DMA software reset below will not complete
 *      otherwise; confirmed against an ST Community report of DMAMR.SWR
 *      hanging when the RMII reference clock isn't already selected and
 *      present at reset time).
 *   2. Issue the DMA software reset and wait for it to clear.
 *   3. Program the station MAC address into MACA0HR/MACA0LR.
 *
 * Caller must have already enabled the SBS and ETH bus clocks
 * (rcc_enable_sbs(), rcc_enable_eth()) and muxed the RMII pins
 * (gpio_configure_rmii_pins()) -- REF_CLK must be live before the reset
 * in step 2 is issued.
 */
void eth_mac_init(const uint8_t mac_addr[6]);

#endif
