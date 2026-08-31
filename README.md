# uNIC — Zero-Copy Ethernet Packet Manager

Bare-metal, register-level Ethernet packet manager for the STM32 Nucleo-H563ZI, treating the board as a miniature NIC: DMA descriptor rings, zero-copy RX/TX buffers, hand-implemented ARP/ICMP/UDP, a programmable match-action packet rule engine, and measured performance (packets/sec, cycles/packet, ring occupancy, drops).

Full proposal: [`projplan.md`](projplan.md).
Current phase plan: [`part1plan.md`](part1plan.md) — Week 1 project skeleton, board bring-up (M1), and the start of PHY link detection (M2).

## Status

Board bring-up (M1) in progress. CMake build produces `unic_firmware.elf/.hex/.bin`.
HSI 64 MHz clock setup and a polled USART3 driver are in place; `main()` emits a
version banner on the ST-Link VCP at 115200 8N1. On-target verification pending.

## Toolchain

- Build: CMake + `arm-none-eabi-gcc` (bundled with STM32CubeIDE 2.1.1)
- Flash/debug: `STM32_Programmer_CLI` (STM32CubeIDE 2.1.1) over ST-Link SWD
- No ST HAL/LL — CMSIS device headers and direct register access only

## Layout

```
firmware/
  drivers/    CMSIS + hand-written peripheral drivers (RCC, GPIO, USART, MDIO/PHY)
  app/        main.c
  linker/     linker script
  net/        packet parsing/protocol handlers (week 2+)
  packet_manager/  rule engine (week 5+)
  telemetry/  stats/counters export (week 4+)
tools/        host-side scripts (flashing, later: traffic generator, dashboard)
docs/         architecture notes, bring-up logs
results/      benchmark csv/plots (week 4+)
tests/hil/    hardware-in-the-loop tests (stretch)
```
