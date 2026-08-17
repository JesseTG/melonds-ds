"""Core options: declaration, reading, updating and visibility."""

from __future__ import annotations

from ctypes import c_char_p, c_uint
from pathlib import Path

import pytest
from libretro import DictOptionDriver
from libretro.ctypes import CStringArg, TypedFunctionPointer

from melondsds import SessionFactory


@pytest.mark.nds_rom
def test_options_version(session: SessionFactory, nds_rom: Path) -> None:
    """The core sees whichever core-options API version the frontend advertises."""
    with session(nds_rom, options=1) as emulator:
        get_options_version = emulator.get_proc_address(
            b"libretropy_get_options_version", TypedFunctionPointer[c_uint, []]
        )
        assert get_options_version is not None
        assert get_options_version() == 1


@pytest.mark.nds_rom
def test_gets_option(session: SessionFactory, nds_rom: Path) -> None:
    """The core reads an option's value, and sees the frontend change it."""
    with session(nds_rom) as emulator:
        get_option = emulator.get_proc_address(
            "libretropy_get_option", TypedFunctionPointer[c_char_p, [CStringArg]]
        )
        assert get_option is not None
        assert get_option(b"melonds_touch_mode") == b"auto"

        emulator.options.variables[b"melonds_touch_mode"] = b"touch"
        assert get_option(b"melonds_touch_mode") == b"touch"


@pytest.mark.nds_rom
def test_option_updates(session: SessionFactory, nds_rom: Path) -> None:
    """Only writing an option raises the updated flag, and running clears it."""
    with session(nds_rom) as emulator:
        assert not emulator.options.variable_updated

        # Reading an option must not mark the variables as updated.
        assert emulator.options.variables["melonds_audio_interpolation"] == b"disabled"
        assert not emulator.options.variable_updated

        # Writing one must.
        emulator.options.variables["melonds_audio_interpolation"] = b"linear"
        assert emulator.options.variable_updated

        # Running the core must clear the flag again.
        emulator.run()
        assert not emulator.options.variable_updated


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("driver", "version", "has_categories"),
    [
        pytest.param(0, 0, False, id="v0"),
        pytest.param(DictOptionDriver(version=1), 1, False, id="v1"),
        # See https://github.com/JesseTG/melonds-ds/issues/51
        pytest.param(None, 2, True, id="v2"),
    ],
)
def test_defines_options(
    session: SessionFactory,
    nds_rom: Path,
    driver: object,
    version: int,
    has_categories: bool,
) -> None:
    """
    The core declares its options at whichever API version the frontend offers.

    ``driver`` of :obj:`None` means "use the frontend's default", which is v2.
    """
    kwargs = {} if driver is None else {"options": driver}

    with session(nds_rom, **kwargs) as emulator:
        assert emulator.options is not None
        assert emulator.options.version == version

        assert emulator.options.definitions is not None
        assert len(emulator.options.definitions) > 0
        assert all(d.key for d in emulator.options.definitions.values())
        definitions = emulator.options.definitions

        if has_categories:
            assert emulator.options.categories is not None
            assert len(emulator.options.categories) > 0
            assert all(c.key for c in emulator.options.categories.values())
        else:
            assert emulator.options.categories is None
            assert all(not d.category_key for d in emulator.options.definitions.values())

    # The frontend's copy of this data must outlive the core.
    del emulator
    assert definitions is not None
    assert len(definitions) > 0
    assert all(v.key for v in definitions.values())


@pytest.mark.nds_rom
def test_options_visibility(session: SessionFactory, nds_rom: Path) -> None:
    """Switching to DSi mode hides the DS-only battery threshold option."""
    with session(nds_rom) as emulator:
        assert emulator.options.update_display_callback
        assert emulator.options.update_display_callback.callback

        assert "melonds_console_mode" in emulator.options.variables
        assert "melonds_dsi_nand_path" in emulator.options.variables

        assert emulator.options.variables["melonds_console_mode"] == b"ds"
        assert emulator.options.visibility["melonds_ds_battery_ok_threshold"]

        emulator.options.variables["melonds_console_mode"] = b"dsi"

        assert emulator.options.variables["melonds_console_mode"] == b"dsi"
        assert not emulator.options.visibility["melonds_ds_battery_ok_threshold"]


@pytest.mark.nds_rom
def test_options_visibility_update_callback(session: SessionFactory, nds_rom: Path) -> None:
    """The core registers a callback for updating option visibility."""
    with session(nds_rom) as emulator:
        assert emulator.options.update_display_callback
        assert emulator.options.update_display_callback.callback
