# uNIC — Week 1 Bring-Up Plan (Project Skeleton + M1 + start of M2)

## Context

`projplan.md` in this directory is a full multi-week proposal for **uNIC**, a bare-metal zero-copy Ethernet packet manager on the STM32 Nucleo-H563ZI, built as a resume-grade firmware/systems portfolio piece (target audience: NVIDIA/Intel/AMD/Microsoft-style firmware/platform/networking teams). The full project spans ~7 weeks (DMA descriptor rings, zero-copy RX/TX, ARP/ICMP/UDP, a programmable packet rule engine, telemetry, Python host tooling). **This plan covers only Week 1** — the project skeleton plus Milestone M1 (board bring-up: UART logs, clock setup, reproducible build/flash) and the start of M2 (PHY link detection) — per the user's explicit request to scope down to "the very first step."

The `unic/` directory is currently empty except for `projplan.md`: fully greenfield, no code, no dedicated repo.

Three foundational decisions were made with the user before designing this plan:
1. **Git**: a brand-new, dedicated repo will be `git init`'d inside `unic/`. Note: `C:\Users\vaibh` (the user's home directory) is itself the root of a large, unrelated pre-existing git repo (a different collaborator's computer-vision/web project). That repo must not be touched or assumed to apply here — `unic/` becomes its own independent repo root.
2. **Build system**: standalone CMake + the `arm-none-eabi-gcc` 14.3.1 toolchain already bundled with the user's installed STM32CubeIDE 2.1.1 (no STM32CubeIDE-managed project, no CubeMX code generation).
3. **Driver style**: bare register-level access via CMSIS device headers only — no ST HAL, no ST LL drivers. This matches the proposal's "register-level reasoning" resume framing.

Environment already confirmed present on this machine:
- `arm-none-eabi-gcc.exe` (v14.3.1) under `C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin\`
- `STM32_Programmer_CLI.exe` (v2.22.0) under `C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\`
- ST-Link server at `C:\Program Files (x86)\STMicroelectronics\stlink_server\stlinkserver.exe`
- No local CMSIS/HAL firmware package cache (`C:\Users\vaibh\STM32Cube\Repository` does not exist) — CMSIS device files must be fetched from ST's GitHub.
- Board confirmed capable: STM32H563ZIT6 (Cortex-M33, 2MB flash / 640KB SRAM) has a real Ethernet MAC/DMA peripheral; the Nucleo-H563ZI carries a LAN8742 PHY over RMII behind the RJ45, wired per RM0481/UM3115.

## Recommended Approach

### 1. Repo skeleton (adapted from proposal §14, deferring weeks 2+ content)

```
unic/
  .git/                          <- git init here, independent of the home-dir repo
  .gitignore
  README.md
  CMakeLists.txt                 (top-level)
  cmake/
    arm-none-eabi-gcc.cmake      (toolchain file, path overridable via cache var)
  firmware/
    CMakeLists.txt
    drivers/
      cmsis/                     (vendored: cmsis-device-h5 + cmsis-core)
      rcc.h / rcc.c
      gpio.h / gpio.c
      usart.h / usart.c
      eth_mdio.h / eth_mdio.c    (Week 1 stretch: MDIO-only PHY register access)
      version.h
    app/
      main.c
    linker/
      STM32H563ZITX_FLASH.ld
    net/            .gitkeep     (empty — week 2+)
    packet_manager/ .gitkeep     (empty — week 2+)
    telemetry/      .gitkeep     (empty — week 2+)
  tools/
    flash.ps1                    (wraps STM32_Programmer_CLI)
  docs/
    week1_bringup.md
  results/
    csv/    .gitkeep
    plots/  .gitkeep
  tests/
    hil/    .gitkeep
```

`.gitignore` excludes `build/`, `*.o/.elf/.hex/.bin/.map/.su`, CMake cache dirs. The toolchain file must accept an `ARM_TOOLCHAIN_BIN` cache-variable override rather than hardcoding the version-hashed CubeIDE plugin path, since that hash changes on IDE updates.

### 2. CMSIS sourcing

Vendor (as git submodules, pinned to a release tag, not `main`) from ST's own GitHub org:
- `https://github.com/STMicroelectronics/cmsis-device-h5` — `Include/stm32h563xx.h`, `Source/Templates/system_stm32h5xx.c`, `Source/Templates/gcc/startup_stm32h563xx.s`
- `https://github.com/STMicroelectronics/cmsis-core` — `core_cm33.h` and other ARM CMSIS-Core headers `stm32h563xx.h` depends on

`https://github.com/STMicroelectronics/STM32CubeH5` is useful as a **read-only reference** (e.g. its Nucleo-H563ZI example projects) for cross-checking register sequences — do not vendor its HAL/LL code.

**Verify at implementation time**: exact startup filename inside `cmsis-device-h5/Source/Templates/gcc/` (expected `startup_stm32h563xx.s` by ST convention, unconfirmed by direct directory listing).

### 3. CMake build

- Toolchain file: `CMAKE_SYSTEM_NAME Generic`, force `CMAKE_C_COMPILER_WORKS`/`CMAKE_ASM_COMPILER_WORKS` TRUE (bare-metal cross-compile, no linkable test executable at configure time), `CMAKE_EXECUTABLE_SUFFIX .elf`.
- Flags: `-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard` (STM32H563 has a hardware single-precision FPU), `-ffunction-sections -fdata-sections`, `-O0 -g3` for Week 1, `--specs=nano.specs --specs=nosys.specs`. TrustZone (`-mcmse`) is not needed for a plain non-secure-only image.
- Linker script `STM32H563ZITX_FLASH.ld`: `FLASH ORIGIN = 0x08000000, LENGTH = 2048K`; `RAM ORIGIN = 0x20000000, LENGTH = 640K` (SRAM1/2/3 treated as one contiguous block for Week 1 — per-region nuances deferred). Standard `.isr_vector/.text/.rodata/.data/.bss` sections, `_estack` symbol.
- Post-link: `arm-none-eabi-objcopy` → `.hex`/`.bin`, `arm-none-eabi-size` for a flash/RAM usage sanity check.
- Skip `printf`/syscall-stub retargeting for Week 1 — hand-roll a blocking `usart3_write_str()` instead.

### 4. Flash + observe

- Flash: `STM32_Programmer_CLI.exe -c port=SWD -w firmware.elf -v -rst`, wrapped in `tools/flash.ps1`.
- **Task 0, blocking, before any code**: `STM32_Programmer_CLI.exe -c port=SWD -ob displ` to check the `TZEN` (TrustZone) option byte. STM32H5 parts are known to ship/behave with TrustZone active by default, which will break a plain non-secure-style bring-up. Disable via option-byte write if set.
- UART logs: ST-Link's virtual COM port enumerates as `COMx`; view with PuTTY/Tera Term at 115200 8N1.

### 5. Clock + UART bring-up (register-level, no HAL)

Stay on **HSI (64 MHz on STM32H5)** for Week 1 — no PLL yet; sufficient for UART and lowest-risk path to the M1 acceptance criterion. Concrete register sequence for USART3 on **PD8 (TX) / PD9 (RX)**, confirmed via two independent sources (Zephyr board DTS + ST community) as the pins wired to the ST-Link VCP on this board:
1. `RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN`
2. `GPIOD->MODER` pins 8/9 → AF mode; `GPIOD->AFR[1]` nibbles for pins 8/9 → **AF7** (standard USART AF across STM32 families; cross-check the datasheet AF table for PD8/PD9 specifically before finalizing)
3. `RCC->APB1LENR |= RCC_APB1LENR_USART3EN` (confirm `APB1LENR` vs `APB1ENR` naming against the fetched header)
4. Confirm USART kernel clock source in `RCC->CCIPR1` before computing `BRR` for 115200 baud
5. `USART3->CR1`: `UE`, `TE`, `RE`; 8N1 is the CR1/CR2 reset default
6. Print `"uNIC fw v0.1.0 | HSI 64MHz | " __DATE__ " " __TIME__` once at boot via polled `TXE`/TDR writes

### 6. PHY link detection (M2 start, Week 1 stretch)

RMII/MDIO pin mapping for NUCLEO-H563ZI (confirmed via ST schematic content + Zephyr DTS, cross-checked twice — note `ETH_TXD1 = PB15` is non-standard vs. the common H7-Nucleo routing, worth an eyeball check against the schematic):

| Signal | Pin | | Signal | Pin |
|---|---|---|---|---|
| ETH_MDC | PC1 | | ETH_TX_EN | PG11 |
| ETH_MDIO | PA2 | | ETH_TXD0 | PG13 |
| ETH_REF_CLK | PA1 | | ETH_TXD1 | PB15 |
| ETH_CRS_DV | PA7 | | | |
| ETH_RXD0 | PC4 | | | |
| ETH_RXD1 | PC5 | | | |

Week 1 only needs MDIO/MDC (PC1, PA2) — a manual MDIO read of the LAN8742's BSR (register 0x01, bit 2 = link status) at PHY address 0 does **not** require the RMII data lines or resolving the REF_CLK source ambiguity (defer that to Week 2 DMA work). Verify the ETH pin alternate-function number (commonly AF11 on H7-style parts, unconfirmed for H5) against the datasheet before finalizing. PHY reset wiring (dedicated GPIO vs. tied to system NRST) is unconfirmed — check the schematic; a software-only reset via PHY BCR register bit 15 works regardless.

## Task Breakdown (Week 1)

0. **Hardware/option-byte sanity** — dump option bytes, disable TrustZone if `TZEN=1`. *Blocking, do first.*
1. **Repo scaffold** — `git init` in `unic/`, create directory skeleton, initial commit.
2. **CMSIS vendoring** — submodule `cmsis-device-h5` + `cmsis-core` into `firmware/drivers/cmsis/`, pinned tags.
3. **CMake build stands up** — toolchain file, linker script, stub `main.c` (empty loop). Acceptance: clean `.elf`/`.hex`/`.bin` build, sane `arm-none-eabi-size` output.
4. **Flash flow works** — `tools/flash.ps1` flashes and resets via `STM32_Programmer_CLI`. Acceptance: verify succeeds, board doesn't hard-fault.
5. **Clock + UART bring-up** — RCC/GPIO/USART3 per §5, print version string. Acceptance: version string observed on `COMx` at 115200 8N1 — this is M1's literal acceptance criterion.
6. **PHY link detection** — MDIO BSR read per §6, print link up/down (and speed/duplex via LAN8742 register 0x1F if verified) over UART. Acceptance: link state visibly changes on cable plug/unplug.
7. **Docs** — `docs/week1_bringup.md` (build/flash instructions, captured UART log, open verification items for Week 2), hardware setup photo — matching the proposal's Week 1 deliverable list.

## Verification

- `cmake --preset <p> && cmake --build <dir>` (or `cmake -G Ninja .. && ninja`) completes with no errors and emits `firmware.elf/.hex/.bin`.
- `tools/flash.ps1` flashes successfully; `STM32_Programmer_CLI` reports verify OK.
- Physical check: open a serial terminal on the enumerated ST-Link `COMx` at 115200 8N1, reset the board, confirm the version string prints.
- Physical check: plug/unplug the Ethernet cable and confirm the printed PHY BSR link-status bit changes accordingly.
- `git log`/`git status` inside `unic/` show a clean, self-contained history that does not touch or depend on the unrelated home-directory repo.
