"""
The virtual SD card that homebrew ROMs see through DLDI.

Most of these cases are negative, but the body is unchanged either way --
it asserts that the card exists,
and the marker says whether that assertion should hold.
"""

from __future__ import annotations

import stat
from pathlib import Path

import pytest

from melondsds import SessionFactory

xfail = pytest.mark.xfail(strict=True, reason="the SD card should not have been created")


def _options(sdcard: str, sync: str) -> dict[str, str]:
    return {
        "melonds_console_mode": "ds",
        "melonds_homebrew_sdcard": sdcard,
        "melonds_homebrew_sync_sdcard_to_host": sync,
    }


@pytest.mark.parametrize(
    ("content_fixture", "options"),
    [
        pytest.param(
            "nds_rom",
            _options("enabled", "disabled"),
            id="retail-enabled",
            marks=[xfail, pytest.mark.nds_rom],
        ),
        pytest.param(
            "nds_rom",
            _options("disabled", "disabled"),
            id="retail-disabled",
            marks=[xfail, pytest.mark.nds_rom],
        ),
        pytest.param(
            "godmode9i_rom",
            _options("enabled", "disabled"),
            id="homebrew-enabled",
            marks=pytest.mark.godmode9i_rom,
        ),
        pytest.param(
            "godmode9i_rom",
            _options("disabled", "disabled"),
            id="homebrew-disabled",
            marks=[xfail, pytest.mark.godmode9i_rom],
        ),
    ],
)
def test_sd_card_image(
    session: SessionFactory,
    dldi_sd_card_path: Path,
    content_fixture: str,
    options: dict[str, str],
    request: pytest.FixtureRequest,
) -> None:
    """The SD card image is created only for homebrew ROMs, and only when enabled."""
    content = request.getfixturevalue(content_fixture)

    with session(content, options=options) as emulator:
        emulator.run()

        sd_card = dldi_sd_card_path.stat()

        assert stat.S_ISREG(sd_card.st_mode)
        assert sd_card.st_size > 0


@pytest.mark.parametrize(
    ("content_fixture", "options"),
    [
        pytest.param(
            "nds_rom",
            _options("enabled", "enabled"),
            id="retail-sync-enabled",
            marks=[xfail, pytest.mark.nds_rom],
        ),
        pytest.param(
            "nds_rom",
            _options("enabled", "disabled"),
            id="retail-sync-disabled",
            marks=[xfail, pytest.mark.nds_rom],
        ),
        pytest.param(
            "godmode9i_rom",
            _options("enabled", "disabled"),
            id="homebrew-sync-disabled",
            marks=[xfail, pytest.mark.godmode9i_rom],
        ),
    ],
)
def test_sd_card_sync_dir(
    session: SessionFactory,
    dldi_sd_card_sync_path: Path,
    content_fixture: str,
    options: dict[str, str],
    request: pytest.FixtureRequest,
) -> None:
    """The host sync directory is created only when SD card syncing is enabled."""
    content = request.getfixturevalue(content_fixture)

    with session(content, options=options) as emulator:
        emulator.run()

        sync_dir = dldi_sd_card_sync_path.stat()

        assert stat.S_ISDIR(sync_dir.st_mode)
