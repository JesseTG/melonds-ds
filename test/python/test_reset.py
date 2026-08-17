"""Resetting the emulated console."""

from __future__ import annotations

from collections.abc import Callable
from itertools import batched
from pathlib import Path

import pytest

from melondsds import SessionFactory
from melondsds.options import system_option_path

FRAMES = 300


@pytest.mark.nds_rom
def test_resets(session: SessionFactory, nds_rom: Path) -> None:
    """``retro_reset`` in the middle of a run doesn't raise."""
    with session(nds_rom) as emulator:
        for _ in range(60):
            emulator.run()

        emulator.reset()

        for _ in range(60):
            emulator.run()


def _builtin_direct() -> dict[str, str]:
    return {
        "melonds_boot_mode": "direct",
        "melonds_console_mode": "ds",
        "melonds_sysfile_mode": "builtin",
    }


def _native_nds(boot_mode: str) -> dict[str, str]:
    return {
        "melonds_boot_mode": boot_mode,
        "melonds_console_mode": "ds",
        "melonds_sysfile_mode": "native",
        "melonds_firmware_nds_path": system_option_path("NDS_FIRMWARE"),
    }


def _native_dsi(boot_mode: str) -> dict[str, str]:
    return {
        "melonds_boot_mode": boot_mode,
        "melonds_console_mode": "dsi",
        "melonds_sysfile_mode": "native",
        "melonds_firmware_dsi_path": system_option_path("DSI_FIRMWARE"),
        "melonds_dsi_nand_path": system_option_path("DSI_NAND"),
    }


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    "make_options",
    [
        pytest.param(_builtin_direct, id="builtin-direct"),
        pytest.param(
            lambda: _native_nds("direct"), id="native-direct", marks=pytest.mark.nds_sysfiles
        ),
        pytest.param(
            lambda: _native_nds("native"), id="native-native", marks=pytest.mark.nds_sysfiles
        ),
        pytest.param(
            lambda: _native_dsi("direct"), id="dsi-direct", marks=pytest.mark.dsi_sysfiles
        ),
        pytest.param(
            lambda: _native_dsi("native"), id="dsi-native", marks=pytest.mark.dsi_sysfiles
        ),
    ],
)
def test_no_hang_on_reboot(session: SessionFactory, nds_rom: Path, make_options: Callable[[], dict[str, str]]) -> None:
    """
    The console keeps rendering after a reset instead of hanging on a blank screen.

    See https://github.com/JesseTG/melonds-ds/issues/62

    ``make_options`` is a callable rather than a plain dict
    because :func:`~melondsds.options.system_option_path` reads the environment,
    which isn't populated while pytest-cmake is collecting tests.
    """
    options = {**make_options(), "melonds_show_cursor": "disabled"}

    with session(nds_rom, options=options) as emulator:
        emulator.run()

        # The very first frame should be a single solid color
        blank_frame = emulator.video.screenshot()
        assert blank_frame is not None
        assert len(set(batched(blank_frame.data, 4))) == 1

        for _ in range(FRAMES):
            emulator.run()

        after_frame = emulator.video.screenshot()
        assert blank_frame != after_frame

        emulator.core.reset()

        for _ in range(FRAMES):
            emulator.core.run()

        after_reset_frame = emulator.video.screenshot()
        assert blank_frame != after_reset_frame
