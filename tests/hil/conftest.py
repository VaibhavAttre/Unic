"""Hardware-in-the-loop fixtures for the Nucleo-H563ZI.

The board must already be flashed (tools\flash.ps1) and connected over the
ST-Link USB. Nothing else may hold the VCP open (CubeIDE, PuTTY, monitor.ps1).

Environment overrides:
  UNIC_PORT         serial port (default: first USB serial device with the
                    ST-Link VID 0x0483)
  UNIC_PROGRAMMER   path to STM32_Programmer_CLI.exe (default: the CubeIDE
                    2.1.1 copy that tools/flash.ps1 uses)
  UNIC_HIL_FLASH=1  flash build/firmware/unic_firmware.elf before the run
  UNIC_HIL_INTERACTIVE=1  enable the cable plug/unplug test (needs a human)
"""
import glob
import os
import re
import subprocess
import time

import pytest
import serial
import serial.tools.list_ports

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ELF = os.path.join(ROOT, "build", "firmware", "unic_firmware.elf")
STLINK_VID = 0x0483
BOOT_CAPTURE_S = 6.0

_DEFAULT_CLI = glob.glob(
    r"C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins"
    r"\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*"
    r"\tools\bin\STM32_Programmer_CLI.exe"
)


def _programmer():
    cli = os.environ.get("UNIC_PROGRAMMER") or (_DEFAULT_CLI[0] if _DEFAULT_CLI else "")
    if not cli or not os.path.exists(cli):
        pytest.skip("STM32_Programmer_CLI.exe not found (set UNIC_PROGRAMMER)")
    return cli


def _port():
    port = os.environ.get("UNIC_PORT")
    if port:
        return port
    for p in serial.tools.list_ports.comports():
        if p.vid == STLINK_VID:
            return p.device
    pytest.skip("no ST-Link virtual COM port found (set UNIC_PORT)")


def _reset_board():
    subprocess.run([_programmer(), "-c", "port=SWD", "-rst"],
                   check=True, capture_output=True, timeout=30)


class Board:
    def __init__(self, port):
        self.port = port

    def boot(self, seconds=BOOT_CAPTURE_S):
        """Reset the MCU and return everything it prints for `seconds`."""
        with serial.Serial(self.port, 115200, timeout=0.2) as s:
            s.reset_input_buffer()
            _reset_board()
            return self._read(s, seconds)

    @staticmethod
    def _read(s, seconds):
        end = time.time() + seconds
        buf = b""
        while time.time() < end:
            buf += s.read(256)
        return buf.decode("ascii", errors="replace")

    def watch(self, until, timeout):
        """Collect serial text (no reset) until `until` appears or timeout."""
        out = ""
        end = time.time() + timeout
        with serial.Serial(self.port, 115200, timeout=0.2) as s:
            while time.time() < end and until not in out:
                out += s.read(256).decode("ascii", errors="replace")
        return out


@pytest.fixture(scope="session")
def board():
    if os.environ.get("UNIC_HIL_FLASH") == "1":
        subprocess.run([_programmer(), "-c", "port=SWD", "-w", ELF, "-v", "-rst"],
                       check=True, capture_output=True, timeout=120)
    return Board(_port())


@pytest.fixture(scope="session")
def boot_log(board):
    return board.boot()


def phy_ids(log):
    """Parse the 'PHY id1=0x.... id2=0x.... bsr=0x....' banner line."""
    m = re.search(r"PHY id1=0x([0-9a-fA-F]{4}) id2=0x([0-9a-fA-F]{4}) bsr=0x([0-9a-fA-F]{4})", log)
    return tuple(int(g, 16) for g in m.groups()) if m else None
