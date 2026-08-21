"""Firmware and BIOS handling: validation, fallback, and not clobbering the originals."""

from __future__ import annotations

from ctypes import c_bool
from pathlib import Path

import pytest
from libretro import DefaultFileSystemDriver, HistoryFileSystemDriver, Open, VfsFileAccess
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.assets import required_asset
from melondsds.options import CORE_SYSTEM_SUBDIR, system_option_path

#: DS mode with the built-in FreeBIOS and firmware.
BUILTIN_DS = {"melonds_console_mode": "ds", "melonds_sysfile_mode": "builtin"}

#: Native DS system files, booting straight into the loaded ROM.
NATIVE_DS_DIRECT = {
    "melonds_console_mode": "ds",
    "melonds_sysfile_mode": "native",
    "melonds_boot_mode": "direct",
}


# --------------------------------------------------------------------------- #
# Falling back to the built-in system files
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    "options",
    [
        pytest.param(NATIVE_DS_DIRECT, id="no-arm7", marks=pytest.mark.arm9_bios),
        pytest.param(NATIVE_DS_DIRECT, id="no-arm9", marks=pytest.mark.arm7_bios),
        pytest.param(
            # Nothing stages the firmware, so this path points at a file that isn't there.
            # `firmware.bin` is the name that find_system_file() searches for.
            {
                **NATIVE_DS_DIRECT,
                "melonds_firmware_nds_path": f"{CORE_SYSTEM_SUBDIR}/firmware.bin",
            },
            id="no-firmware",
            marks=[pytest.mark.arm7_bios, pytest.mark.arm9_bios],
        ),
    ],
)
def test_falls_back_to_freebios(
    session: SessionFactory, nds_rom: Path, options: dict[str, str]
) -> None:
    """With an incomplete set of native system files, the core uses the built-in ones."""
    with session(nds_rom, options=options) as emulator:
        proc_address_callback = emulator.proc_address_callback
        assert proc_address_callback is not None
        assert proc_address_callback.get_proc_address is not None

        console_exists = emulator.get_proc_address("melondsds_console_exists", TypedFunctionPointer[c_bool, []])
        assert console_exists is not None
        assert console_exists()

        for symbol in (
            "melondsds_arm7_bios_native",
            "melondsds_arm9_bios_native",
            "melondsds_firmware_native",
        ):
            probe = emulator.get_proc_address(symbol, TypedFunctionPointer[c_bool, []])
            assert probe is not None
            assert not probe()


@pytest.mark.nds_rom
def test_native_bios_not_loaded_with_freebios(session: SessionFactory, nds_rom: Path) -> None:
    """The core doesn't even try to open the native BIOS when using FreeBIOS."""
    vfs = HistoryFileSystemDriver(DefaultFileSystemDriver())

    with session(nds_rom, vfs=vfs, options=BUILTIN_DS) as emulator:
        for _ in range(300):
            emulator.run()

        for op in vfs.history:
            if not isinstance(op, Open):
                continue

            assert isinstance(op.path, bytes)
            assert not op.path.endswith(b"bios7.bin")
            assert not op.path.endswith(b"bios9.bin")


# --------------------------------------------------------------------------- #
# Validating firmware images
# See https://github.com/JesseTG/melonds-ds/issues/183
# --------------------------------------------------------------------------- #


@pytest.mark.nds_sysfiles
def test_rejects_wrong_sized_firmware(session: SessionFactory) -> None:
    """A file that isn't firmware-sized is not offered as an NDS firmware choice."""
    arm9_name = required_asset("ARM9_BIOS").name.encode()

    with session() as emulator:
        definitions = emulator.options.definitions
        assert definitions
        definition = definitions[b"melonds_firmware_nds_path"]
        assert definition is not None

        # The values array has empty trailing entries, hence the `is None` check.
        assert all(v.value is None or arm9_name not in v.value for v in definition.values)


@pytest.mark.nds_sysfiles
def test_rejects_invalid_firmware_id(session: SessionFactory, core_system_dir: Path) -> None:
    """Firmware with a corrupted identifier is not offered as an NDS firmware choice."""
    firmware = bytearray(required_asset("NDS_FIRMWARE").read_bytes())
    assert len(firmware) == 262144

    firmware[0x8:0xC] = b"NNNN"
    (core_system_dir / "badfirmware.bin").write_bytes(firmware)

    with session() as emulator:
        definitions = emulator.options.definitions
        assert definitions
        definition = definitions.get(b"melonds_firmware_nds_path")
        assert definition

        # The values array has empty trailing entries, hence the `is None` check.
        assert all(v.value is None or b"badfirmware.bin" not in v.value for v in definition.values)


# --------------------------------------------------------------------------- #
# Not overwriting the images the user supplied
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_saves_wfcsettings(session: SessionFactory, nds_rom: Path, wfcsettings_path: Path) -> None:
    """Wi-Fi settings are written to wfcsettings.bin when using the built-in firmware."""
    assert not wfcsettings_path.exists()

    options = {**BUILTIN_DS, "melonds_firmware_nds_path": "/builtin"}
    with session(nds_rom, options=options) as emulator:
        for _ in range(300):
            emulator.run()

        assert wfcsettings_path.exists()


@pytest.mark.nds_rom
def test_loads_wfcsettings(session: SessionFactory, nds_rom: Path, wfcsettings_path: Path) -> None:
    """The core opens wfcsettings.bin for writing when using the built-in firmware."""
    assert not wfcsettings_path.exists()

    vfs = HistoryFileSystemDriver(DefaultFileSystemDriver())
    with session(nds_rom, vfs=vfs, options=BUILTIN_DS) as emulator:
        for _ in range(300):
            emulator.run()

        assert any(
            op.path.endswith(b"wfcsettings.bin") and VfsFileAccess.WRITE in op.mode
            for op in vfs.history
            if isinstance(op, Open) and op.result is not None
        )


@pytest.mark.nds_sysfiles
def test_nds_firmware_not_overwritten(session: SessionFactory, core_system_dir: Path) -> None:
    """
    Saving Wi-Fi settings doesn't grow or truncate the native firmware image.

    See https://github.com/JesseTG/melonds-ds/issues/59
    """
    source = required_asset("NDS_FIRMWARE")
    staged = core_system_dir / source.name

    original_size = source.stat().st_size
    assert original_size > 0

    options = {
        "melonds_console_mode": "ds",
        "melonds_sysfile_mode": "native",
        "melonds_boot_mode": "native",
        "melonds_firmware_nds_path": system_option_path("NDS_FIRMWARE"),
    }

    with session(options=options) as emulator:
        for _ in range(300):
            emulator.run()

        assert staged.stat().st_size == original_size


@pytest.mark.nds_rom
@pytest.mark.nds_sysfiles
@pytest.mark.dsi_sysfiles
@pytest.mark.parametrize("console_mode", ["ds", "dsi"])
def test_firmware_not_overwritten_on_console_switch(
    session: SessionFactory, nds_rom: Path, core_system_dir: Path, console_mode: str
) -> None:
    """Switching console mode and resetting doesn't clobber either firmware image."""
    nds_source = required_asset("NDS_FIRMWARE")
    dsi_source = required_asset("DSI_FIRMWARE")

    nds_firmware = nds_source.read_bytes()
    dsi_firmware = dsi_source.read_bytes()
    assert nds_firmware != dsi_firmware

    options = {
        "melonds_console_mode": console_mode,
        "melonds_sysfile_mode": "native",
        "melonds_boot_directly": "false",
        "melonds_firmware_nds_path": system_option_path("NDS_FIRMWARE"),
        "melonds_firmware_dsi_path": system_option_path("DSI_FIRMWARE"),
        "melonds_dsi_nand_path": system_option_path("DSI_NAND"),
    }
    other_mode = "dsi" if console_mode == "ds" else "ds"

    with session(nds_rom, options=options) as emulator:
        for _ in range(30):
            emulator.run()

        emulator.options.variables["melonds_console_mode"] = other_mode.encode()
        emulator.reset()

        for _ in range(30):
            emulator.run()

    nds_after = (core_system_dir / nds_source.name).read_bytes()
    dsi_after = (core_system_dir / dsi_source.name).read_bytes()

    assert len(nds_after) == len(nds_firmware)
    assert len(dsi_after) == len(dsi_firmware)
    assert nds_after != dsi_after
