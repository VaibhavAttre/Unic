# uNIC — Week 2 Plan (RMII/MAC bring-up, RX/TX DMA rings, ARP + ICMP)

## Context

`projplan.md` in this directory is a full multi-week proposal for **uNIC**, a
bare-metal zero-copy Ethernet packet manager on the STM32 Nucleo-H563ZI. Week 1
(`part1plan.md`) delivered the project skeleton, Milestone **M1** (board
bring-up: HSI 64 MHz clock, polled USART3, version banner) and the start of
**M2** (PHY link detection via a bit-banged MDIO master), both verified on
hardware and logged in [`docs/week1_bringup.md`](docs/week1_bringup.md).

Per `projplan.md`'s timeline (§12), Week 2 is scoped as *"RX/TX descriptors,
raw frame receive/transmit, ARP and ICMP"* — milestones **M3** (RX ring), **M4**
(TX ring), and **M5** (ARP + ICMP) from §11, with deliverable *"PC can ping
board; Wireshark capture validates packets."*

None of that exists yet: the Ethernet MAC, DMA, and RMII data path have not
been touched — Week 1 deliberately used only MDC/MDIO (PC1/PA2) to read the PHY
directly, bypassing the MAC entirely. `firmware/net/`, `firmware/packet_manager/`,
and `firmware/telemetry/` are still empty `.gitkeep` placeholders, and the only
Ethernet-adjacent code is the bit-banged `firmware/drivers/eth_mdio.c/.h` and
the link-polling loop in `firmware/app/main.c`.

Week 1's "Open items for Week 2" list five prerequisites that sit in front of
M3/M4/M5 and must land first:
1. Move PHY management off the bit-banged GPIO MDIO master onto the MAC's
   hardware station-management interface.
2. Confirm the ETH signal alternate-function number (expected AF11) against
   the datasheet.
3. Confirm `ETH_TXD1 = PB15` against the board schematic.
4. Resolve the `ETH_REF_CLK` source (external 50 MHz oscillator on PA1 vs.
   MCO-generated) before wiring the RMII data path.
5. Determine PHY reset wiring (dedicated GPIO vs. tied to system NRST); a
   software reset via PHY BCR bit 15 works regardless.
6. Move off HSI to a PLL clock configuration once RMII timing needs it.

This plan folds those prerequisites into Week 2's task list ahead of M3/M4/M5,
since RX/TX frames cannot move without the RMII data path and MAC being live.

Register-level grounding was confirmed directly against the vendored CMSIS
header (`firmware/drivers/cmsis/cmsis-device-h5/Include/stm32h563xx.h`), not
assumed from other STM32 families:

- The STM32H5 Ethernet block is **single-channel** DesignWare-style MAC/DMA
  (unlike H7's multi-channel `DMACx[]` array). `ETH_TypeDef` has one
  `DMACCR`/`DMACTCR`/`DMACRCR`, one `DMACTDLAR`/`DMACRDLAR` (descriptor list
  base), `DMACTDTPR`/`DMACRDTPR` (tail pointer), `DMACTDRLR`/`DMACRDRLR` (ring
  length), `DMACIER`, `DMACSR`.
- Hardware MDIO is `MACMDIOAR`/`MACMDIODR` — this replaces the bit-banged
  transfer primitive in `eth_mdio.c`.
- Clock gating: `RCC_AHB1ENR_ETHEN` / `ETHTXEN` / `ETHRXEN`.
- RMII vs. MII select is `SBS_PMCR_ETH_SEL_PHY` (STM32H5 uses the SBS block for
  this, not `SYSCFG` as on older families); `PWR_PMCR_ETHERNETSO` also exists
  and should be checked during implementation for an Ethernet supply-related
  setting on this line.
- MAC address registers: `MACA0HR`/`MACA0LR`. Frame filter: `MACPFR`.

## Recommended Approach

### 1. Clock: move to PLL

HSI 64 MHz (Week 1) is sufficient for UART and bit-banged GPIO but not
justified against Ethernet's kernel-clock/RMII timing needs. Reconfigure the
system clock via the PLL to a higher AHB frequency, and settle how the RMII
50 MHz reference clock is sourced (see §2) since that choice feeds directly
into the PLL/MCO configuration decision. Touches `rcc.c`/`rcc.h` only —
`usart.c`'s baud-rate calculation already takes `RCC_PCLK1_HZ` as a parameter,
so it does not need to change, only the value passed to it.

### 2. RMII pin bring-up

Configure alternate-function mode for the 8 RMII signals identified in
`part1plan.md` §6:

| Signal | Pin | | Signal | Pin |
|---|---|---|---|---|
| ETH_MDC | PC1 | | ETH_TX_EN | PG11 |
| ETH_MDIO | PA2 | | ETH_TXD0 | PG13 |
| ETH_REF_CLK | PA1 | | ETH_TXD1 | PB15 |
| ETH_CRS_DV | PA7 | | | |
| ETH_RXD0 | PC4 | | | |
| ETH_RXD1 | PC5 | | | |

(MDC/MDIO are already configured as plain GPIO by `eth_mdio_init()`; they get
reconfigured to AF mode here alongside the rest.) Before finalizing:
resolve the AF number (expected AF11, confirm against the H563 datasheet AF
table — unconfirmed in Week 1), verify `ETH_TXD1 = PB15` against the board
schematic (flagged as non-standard vs. H7-Nucleo routing), and resolve
`ETH_REF_CLK`'s source. New function: `gpio_configure_rmii_pins()` in
`gpio.c`/`gpio.h`, following the existing `gpio_configure_usart3_pins()`
pattern.

### 3. MAC init + hardware MDIO

- Enable `RCC_AHB1ENR_ETHEN`, `ETHTXEN`, `ETHRXEN`.
- Set `SBS_PMCR_ETH_SEL_PHY` for RMII before any other MAC register access
  (interface selection must happen before the MAC is enabled).
- Program `MACA0HR`/`MACA0LR` with a chosen locally-administered MAC address.
- New `firmware/drivers/eth_mac.c`/`eth_mac.h` for MAC-level init
  (clock/interface-select/address/`MACCR` configuration).
- Swap `eth_mdio.c`'s bit-banged transfer for `MACMDIOAR`/`MACMDIODR`
  (busy-bit polled Clause 22 transaction). Keep `eth_mdio.h`'s public API
  (`eth_mdio_init`/`eth_mdio_read`/`eth_mdio_write`) unchanged so `main.c`'s
  existing link/auto-neg polling logic and the `PHY_REG_*`/`PHY_BSR_*` decode
  constants need no changes — only the transfer primitive underneath swaps
  out. This also removes the bit-bang timing margin behind Week 1's
  `id2=0xc131` misread.

### 4. DMA descriptor rings (M3 RX, M4 TX)

New `firmware/drivers/eth_dma.c`/`eth_dma.h`:
- Normal (non-timestamp, non-context) descriptor struct layout.
- Small fixed-count statically-allocated rings with buffers (e.g. 4 RX / 2 TX
  descriptors — enough to demonstrate ownership cycling without needing
  dynamic allocation).
- Program `DMACRDLAR`/`DMACTDLAR` (ring base), `DMACRDRLR`/`DMACTDRLR` (ring
  length), initialize descriptor OWN bits, start `DMACRCR`/`DMACTCR` and the
  global `DMACCR`.
- OWN-bit ownership protocol for handoff between CPU and DMA; bump
  `DMACRDTPR`/`DMACTDTPR` (tail pointer) to hand a descriptor to DMA.
- Note explicitly whether a data-cache sits between the CPU and the
  descriptor/buffer SRAM on this bus path (H563 M33 core) and, if so, add
  `__DSB()`/cache-maintenance around ownership handoff. Flag as unconfirmed
  if not resolved before implementation, matching Week 1's practice of
  documenting open hardware assumptions rather than guessing silently.

### 5. RX path (M3)

Poll `DMACSR` for RX-complete (interrupt-driven RX is out of scope until the
Week 4+ interrupt-vs-polling benchmark). On completion: read frame length and
EtherType directly from the completed descriptor's buffer (14-byte Ethernet
header only — no protocol parsing yet), print over UART, then recycle the
descriptor (clear/reset OWN bit, advance tail pointer). M3 acceptance:
firmware receives raw frames and prints frame length/EtherType counters.

### 6. TX path (M4)

Build one hand-crafted test Ethernet frame directly in a TX buffer (fixed
destination MAC, e.g. broadcast), hand it to the TX ring via `DMACTDLAR`/tail
pointer, confirm completion via `DMACSR`, recycle the descriptor. M4
acceptance: firmware can send a raw Ethernet frame or test packet.

### 7. ARP responder (M5, part 1)

New `firmware/net/arp.c`/`arp.h`: parse an incoming ARP request in place off
the RX descriptor buffer (§5), build an ARP reply directly in a TX buffer
using the board's fixed static IP and MAC address, transmit via the M4 TX
path (§6). No dynamic ARP cache needed — replies only.

### 8. ICMP echo responder (M5, part 2)

- New `firmware/net/ethernet.c`/`ethernet.h`: shared Ethernet II header
  parse/build helpers used by both ARP and ICMP/IPv4.
- New `firmware/net/ipv4.c`/`ipv4.h`: header validation (version, IHL, total
  length, protocol) and header checksum compute/verify.
- New `firmware/net/icmp.c`/`icmp.h`: echo request → echo reply, with
  checksum recomputation.

This is where `firmware/net/` stops being an empty `.gitkeep` placeholder.

## Task Breakdown (Week 2)

0. **Clock: PLL reconfig** — `rcc.c`/`rcc.h`. Blocking prerequisite; decide
   the RMII reference-clock source (§2) first since it constrains the PLL/MCO
   choice.
1. **RMII pin AF config** — new `gpio_configure_rmii_pins()` in
   `gpio.c`/`gpio.h`. Resolve AF number, `ETH_TXD1` pin, and `ETH_REF_CLK`
   source against the datasheet/schematic before finalizing.
2. **MAC init** — new `eth_mac.c`/`eth_mac.h`: clock enables, `SBS_PMCR_ETH_SEL_PHY`,
   `MACA0HR`/`MACA0LR` address programming.
3. **Hardware MDIO swap** — `eth_mdio.c` transfer primitive moves to
   `MACMDIOAR`/`MACMDIODR`; public API and `main.c` unchanged. Acceptance:
   PHY id1/id2/BSR reads still match Week 1's values.
4. **RX descriptor ring** — new `eth_dma.c`/`eth_dma.h`: RX ring init,
   raw-frame receive + length/EtherType print. Acceptance (M3): firmware
   receives raw frames and prints frame length/EtherType counters.
5. **TX descriptor ring** — TX ring init, hand-built test frame send.
   Acceptance (M4): firmware can send a raw Ethernet frame or test packet,
   observable in Wireshark on the PC.
6. **ARP responder** — `firmware/net/arp.c`/`arp.h`. Acceptance: PC `arp -a`
   resolves the board's MAC for its static IP.
7. **ICMP echo responder** — `firmware/net/ethernet.c`, `ipv4.c`, `icmp.c`.
   Acceptance (M5): `ping <board-ip>` from the PC succeeds.
8. **Docs** — `docs/week2_bringup.md` (written after implementation, matching
   the `docs/week1_bringup.md` template: hardware/pin tables, captured UART
   log, Wireshark capture note, open items for Week 3). Not written as part
   of this planning task.

## Verification

- `cmake --preset default && cmake --build --preset default` completes with
  no errors and emits `unic_firmware.elf/.hex/.bin` (build/flash flow
  unchanged from Week 1).
- `tools\flash.ps1` flashes successfully; `STM32_Programmer_CLI` reports
  verify OK, board does not hard-fault.
- Serial monitor (`tools\monitor.ps1`) shows link-up/auto-neg reporting still
  working after the hardware-MDIO swap (Task 3), then RX/TX descriptor
  activity as frames are received/sent (Tasks 4-5).
- Wireshark on the PC NIC connected to the board: confirms well-formed
  Ethernet frames for the M4 test send, a correct ARP reply, and a correct
  ICMP echo reply.
- `arp -a` on the PC resolves the board's MAC address for its static IP
  (M5, ARP half).
- `ping <board-ip>` from the PC succeeds (M5 acceptance criterion, verbatim
  from `projplan.md` §11).
- UART log shows RX/TX descriptor recycling without stalls across repeated
  pings — no descriptor-ownership deadlock under sustained traffic.

## Open items carried into Week 3

- Interrupt-driven RX (currently polled `DMACSR`) — deferred to the Week 4+
  interrupt-vs-polling benchmark per `projplan.md` §10.
- Zero-copy cleanup pass (M7) is out of scope for Week 2; RX parsing already
  happens in place off the DMA buffer by construction (§5/§7/§8), but the
  explicit "no `memcpy` in the RX fast path" audit is a later milestone.
- UDP echo/sink (M6) and telemetry (M8) remain Week 3 per `projplan.md`'s
  timeline.
