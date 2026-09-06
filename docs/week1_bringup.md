# Week 1 Bring-Up Log

Board bring-up (M1) and the start of PHY link detection (M2) on the STM32
Nucleo-H563ZI, built bare-metal with CMake + `arm-none-eabi-gcc` and direct
register access (no ST HAL/LL).

## Hardware

| Item | Detail |
|---|---|
| Board | ST NUCLEO-H563ZI |
| MCU | STM32H563ZIT6 — Cortex-M33, 2 MB flash, 640 KB SRAM |
| PHY | Microchip LAN8742A, RMII, station address 0 |
| Debug/UART | On-board ST-Link, SWD + Virtual COM Port (115200 8N1) |
| Clock | HSI 64 MHz, all prescalers /1 (no PLL yet) |

### Pin map in use

| Function | Pin | Mode |
|---|---|---|
| USART3 TX | PD8 | AF7 |
| USART3 RX | PD9 | AF7 |
| ETH_MDC | PC1 | GPIO output, push-pull |
| ETH_MDIO | PA2 | GPIO output, open-drain + pull-up |

MDC/MDIO are bit-banged as an IEEE 802.3 Clause 22 station-management master;
the Ethernet MAC and the RMII data path are not brought up in Week 1.

## Prerequisites

- `cmake` on `PATH` (3.25+).
- STM32CubeIDE 2.1.1 installed at the default location. The toolchain, `ninja`,
  and `STM32_Programmer_CLI` paths are baked into `cmake/arm-none-eabi-gcc.cmake`,
  `CMakePresets.json`, and `tools/flash.ps1`; override with the documented cache
  variable / script parameter if a CubeIDE update changes the plugin hashes.
- CMSIS headers are vendored as submodules — after a fresh clone:
  `git submodule update --init --recursive`.

## Build

```
cmake --preset default
cmake --build --preset default
```

Outputs land in `build/firmware/`:

- `unic_firmware.elf` — flashed image
- `unic_firmware.hex`, `unic_firmware.bin` — generated post-link
- `unic_firmware.map` — link map

`arm-none-eabi-size` runs as a post-build step. Current footprint (`-O0 -g3`):

```
   text    data     bss     dec     hex
   ~3.1k       0   ~1.6k   ~4.7k    ~b70
```

The newlib `_close/_lseek/_read/_write` "not implemented" link warnings are
expected — those stubs are pulled in by `nosys.specs` and garbage-collected out.

## Flash

```
tools\flash.ps1
```

Wraps `STM32_Programmer_CLI -c port=SWD -w <elf> -v -rst`. Verify must report OK
and the board must reset into the new image without hard-faulting.

## Serial monitor

```
tools\monitor.ps1 -Port COM3
```

Auto-detection keys off a PnP name containing "Virtual COM"; on this machine the
ST-Link VCP does not use that name, so `-Port` must be passed explicitly (check
Device Manager for the current COM number). Press the black NRST button to see
the boot banner.

## Captured boot output

Cable out at power-on, then plugged in (`Sep 6 2026` build):

```
uNIC fw v0.1.0 | HSI 64MHz | Sep  6 2026 08:53:35
reset: soft pin
PHY id1=0x0007 id2=0xc131 bsr=0x7809
link down
link up
autoneg: done 100/full lpa=0xc5e1
```

Field notes:

- `reset:` decodes `RCC->RSR` and then clears the flags, so each boot reports
  only its own cause. `pin` = external NRST (NRST button or ST-Link); `soft` =
  the system reset `STM32_Programmer_CLI -rst` issues after programming. Both are
  expected right after a flash. Repeated resets with no button press would point
  at the USB cable / hub / an open debug session, not the firmware.
- `PHY id1` must read `0x0007` for a LAN8742. `id2` should read `0xc0fx` (upper
  12 bits fixed, low nibble = silicon revision); the `0xc131` seen here is a
  stable MDIO misread of that register — see MDIO implementation notes.
- `bsr` is PHY register 0x01 (Basic Status). Bit 2 = link up, bit 5 = auto-neg
  complete. `0x7809` = link down, auto-neg not complete — correct with the cable
  out.
- The link line prints only on a change of state. It is a 5-sample majority
  vote of the BSR link bit at a 200 ms poll cadence, so a single corrupted MDIO
  read cannot fake a transition.
- `autoneg:` prints once per link-up. It waits (bounded at 3 s) for `BSR` bit 5,
  then decodes the resolved speed/duplex from the LAN8742 vendor register 0x1F
  (bits 4:2) and the link partner's advertised abilities from `ANLPAR` (0x05).
  `lpa=0xc5e1` advertises 10FD/100HD/100FD + pause, so the `100/full` result is
  the genuine highest common mode, not a fallback. This speed/duplex pair is
  what the Ethernet MAC's `MACCR` FES/DM bits must be set to when the RMII data
  path is brought up — RMII carries no in-band speed signalling.

## MDIO implementation notes

- One turnaround clock after the read command, 16 data bits, then one trailing
  idle clock with MDIO released. Two turnaround clocks shift the result left by
  one bit (observed `0x0007` reading back as `0x000f`). The trailing idle clock
  returns the bus to a known state before the next frame's preamble and fixed
  an intermittent `0xffff` on back-to-back reads.
- `MDIO_HALF_PERIOD_SPINS = 16` gives MDC roughly 250 kHz at `-O0`, well under
  the LAN8742's 2.5 MHz limit. Raise it first if reads stay flaky.
- `id2` reads a stable `0xc131` rather than `~0xc0f1`. It is reproducible and
  localised to bits 6-8, which sit at a data transition in that register's value;
  `id1` (all-zero upper bits) and the registers used for link detection and
  auto-neg reporting are unaffected. Left as-is for now; slowing MDC is the
  expected fix if the RMII bring-up needs `id2` reliable.

## Verification status

| Task | Acceptance | Status |
|---|---|---|
| 3 — CMake build | Clean `.elf/.hex/.bin`, sane size | Pass |
| 4 — Flash flow | Verify OK, no hard-fault | Pass |
| 5 — Clock + UART (M1) | Version banner on VCP at 115200 8N1 | Pass, on hardware |
| 6 — PHY link detect (M2) | Link state changes on cable plug/unplug | Pass, on hardware |

Task 6 notes:

- `id1=0x0007` reads back correctly after the turnaround fix.
- `link up`/`link down` tracks physical cable plug/unplug across repeated cycles.
- On each `link up`, auto-negotiation completes within the 3 s bound and reports
  `100/full` with a valid `ANLPAR` — the link is fully usable, not just
  electrically present.
- Remaining cosmetic issue: `id2` reads a stable `0xc131` instead of `~0xc0f1`.
  The reads that drive link detection (`BSR`) and the auto-neg report (`PSCSR`,
  `ANLPAR`) all come back clean, so this is deferred rather than chased.

## Open items for Week 2

- Chase down the stable `id2=0xc131` misread if the RMII MDIO path needs it —
  first lever is raising `MDIO_HALF_PERIOD_SPINS` (slower MDC).
- Confirm the ETH signal alternate-function number against the STM32H563
  datasheet AF table (expected AF11) before wiring the RMII data path.
- Confirm `ETH_TXD1 = PB15` against the board schematic — non-standard versus
  the H7-Nucleo routing.
- Resolve the ETH_REF_CLK source (PA1 external vs. MCO) for the RMII 50 MHz
  reference before DMA bring-up.
- Determine PHY reset wiring (dedicated GPIO vs. tied to system NRST); a
  software reset via PHY BCR bit 15 works regardless.
- Move off HSI to a PLL clock configuration once RMII timing needs it.

## Hardware setup photo

_TODO: add a photo of the board with the SWD/USB and Ethernet cables connected
(`docs/img/week1_setup.jpg`)._
