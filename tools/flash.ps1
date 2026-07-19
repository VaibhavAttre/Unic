<#
.SYNOPSIS
    Flash uNIC firmware to the Nucleo-H563ZI over SWD via STM32_Programmer_CLI.

.PARAMETER ElfPath
    Path to the firmware .elf to flash. Defaults to the standard CMake build
    output location.

.PARAMETER ProgrammerCli
    Path to STM32_Programmer_CLI.exe. Defaults to the copy bundled with this
    machine's STM32CubeIDE 2.1.1 install. That path embeds a version-hashed
    plugin directory that will change on a CubeIDE update - override with
    this parameter rather than editing the script.

.EXAMPLE
    tools\flash.ps1
    tools\flash.ps1 -ElfPath build\firmware\unic_firmware.elf
#>
param(
    [string]$ElfPath = (Join-Path $PSScriptRoot "..\build\firmware\unic_firmware.elf"),
    [string]$ProgrammerCli = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
)

if (-not (Test-Path $ProgrammerCli)) {
    Write-Error "STM32_Programmer_CLI.exe not found at '$ProgrammerCli'. Pass -ProgrammerCli <path> to override (e.g. after a CubeIDE update changes the plugin hash)."
    exit 1
}

if (-not (Test-Path $ElfPath)) {
    Write-Error "Firmware not found at '$ElfPath'. Build it first: cmake --build --preset default"
    exit 1
}

& $ProgrammerCli -c port=SWD -w $ElfPath -v -rst
exit $LASTEXITCODE
