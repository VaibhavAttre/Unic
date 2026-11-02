"""HIL tests for PHY management (MDIO) on the LAN8742A.

These exercise the MAC's hardware station-management path in
firmware/drivers/eth_mdio.c through the boot log that firmware/app/main.c
prints. Run from the repo root:

    python -m pytest tests/hil -v
"""
import os
import re

import pytest

from conftest import phy_ids

LAN8742_ID1 = 0x0007
LAN8742_ID2_MASK = 0xFFF0   # low nibble is the silicon revision
LAN8742_ID2 = 0xC130

BSR_EXT_CAPABLE = 1 << 0
BSR_AUTONEG_ABLE = 1 << 3


def test_boot_banner(boot_log):
    """M1: clock + UART alive after the PLL / MAC changes."""
    assert re.search(r"uNIC fw v\d+\.\d+\.\d+ \| PLL 250MHz", boot_log), boot_log


def test_mac_reset_completes(boot_log):
    """eth_mac_init() hangs in DMAMR.SWR without a live ETH_REF_CLK, so
    getting past it to the PHY line proves PA1 is receiving the PHY's 50 MHz."""
    assert "PHY id1=" in boot_log, "firmware never got past eth_mac_init():\n" + boot_log


def test_phy_id1(boot_log):
    ids = phy_ids(boot_log)
    assert ids, boot_log
    assert ids[0] == LAN8742_ID1


def test_phy_id2_is_lan8742a(boot_log):
    ids = phy_ids(boot_log)
    assert ids, boot_log
    assert ids[1] & LAN8742_ID2_MASK == LAN8742_ID2, f"id2=0x{ids[1]:04x}"


def test_bsr_is_sane(boot_log):
    """BSR must not be a floating-bus (0x0000/0xFFFF) read and must advertise
    extended-register and auto-negotiation capability."""
    ids = phy_ids(boot_log)
    assert ids, boot_log
    bsr = ids[2]
    assert bsr not in (0x0000, 0xFFFF)
    assert bsr & BSR_EXT_CAPABLE
    assert bsr & BSR_AUTONEG_ABLE


def test_mdio_reads_are_stable_across_resets(board):
    """The Week 1 bit-bang drifted between runs; hardware MDIO must not."""
    seen = {phy_ids(board.boot())[:2] for _ in range(5)}
    assert len(seen) == 1, f"PHY id1/id2 varied across resets: {seen}"


def test_link_and_autoneg_report(boot_log):
    """Needs the Ethernet cable connected to a live partner at boot."""
    if "link up" not in boot_log:
        pytest.skip("no link at boot (cable unplugged?)")
    m = re.search(r"autoneg: done (10|100)/(full|half) lpa=0x([0-9a-fA-F]{4})", boot_log)
    assert m, "link came up but no 'autoneg: done' line:\n" + boot_log
    lpa = int(m.group(3), 16)
    assert lpa & 0x1F == 0x01, f"ANLPAR selector field is not IEEE 802.3: 0x{lpa:04x}"


@pytest.mark.skipif(os.environ.get("UNIC_HIL_INTERACTIVE") != "1",
                    reason="set UNIC_HIL_INTERACTIVE=1 (needs a human at the cable)")
def test_link_tracks_cable(board):
    board.boot()
    for _ in range(3):
        input("\nUNPLUG the Ethernet cable, then press Enter... ")
        assert "link down" in board.watch("link down", 10), "no 'link down' after unplug"
        input("PLUG the Ethernet cable back in, then press Enter... ")
        out = board.watch("autoneg:", 15)
        assert "link up" in out, "no 'link up' after replug"
        assert "autoneg: done" in out, "link up without an auto-negotiation result"
