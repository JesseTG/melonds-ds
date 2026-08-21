"""
Environment calls: directories, capabilities, messages and metadata.

These used to double as tests for libretro.py itself,
which is why several of them call back into the core
through ``libretropy_*`` helpers rather than melonDS DS's own.
"""

from __future__ import annotations

import logging
from ctypes import byref, c_bool, c_char_p, c_int, c_uint, c_uint64
from pathlib import Path
from pprint import pformat

import pytest
from libretro import (
    DefaultFileSystemDriver,
    HistoryFileSystemDriver,
    LoggerMessageDriver,
    PowerState,
    retro_device_power,
)
from libretro.ctypes import CIntArg, CStringArg, TypedFunctionPointer, TypedPointer

from melondsds import SessionFactory

# --------------------------------------------------------------------------- #
# Directories
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_gets_system_directory(session: SessionFactory, nds_rom: Path) -> None:
    """The core reads back the same system directory the frontend gave it."""
    with session(nds_rom) as emulator:
        get_system_directory = emulator.get_proc_address(
            b"libretropy_get_system_directory", TypedFunctionPointer[c_char_p, []]
        )
        assert get_system_directory is not None

        assert get_system_directory() == emulator.system_dir


@pytest.mark.nds_rom
def test_gets_save_directory(session: SessionFactory, nds_rom: Path) -> None:
    """The core reads back the same save directory the frontend gave it."""
    with session(nds_rom) as emulator:
        get_save_directory = emulator.get_proc_address(
            b"libretropy_get_save_directory", TypedFunctionPointer[c_char_p, []]
        )
        assert get_save_directory is not None

        assert get_save_directory() == emulator.save_dir


# --------------------------------------------------------------------------- #
# Capabilities
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_proc_address(session: SessionFactory, nds_rom: Path) -> None:
    """The core exposes ``retro_get_proc_address_interface`` and it works."""
    with session(nds_rom) as emulator:
        proc_address_callback = emulator.proc_address_callback
        assert proc_address_callback is not None
        assert proc_address_callback.get_proc_address is not None

        add_integers = emulator.get_proc_address(
            b"libretropy_add_integers", TypedFunctionPointer[c_int, [CIntArg[c_int], CIntArg[c_int]]]
        )
        assert add_integers is not None
        assert add_integers(1, 2) == 3


@pytest.mark.nds_rom
def test_input_bitmask_support(session: SessionFactory, nds_rom: Path) -> None:
    """The core sees that the frontend supports input bitmasks."""
    with session(nds_rom) as emulator:
        get_input_bitmasks = emulator.get_proc_address(
            "libretropy_get_input_bitmasks", TypedFunctionPointer[c_bool, []]
        )
        assert get_input_bitmasks is not None
        assert get_input_bitmasks()


@pytest.mark.nds_rom
def test_input_capabilities(session: SessionFactory, nds_rom: Path) -> None:
    """The core sees a non-empty set of input device capabilities."""
    with session(nds_rom) as emulator:
        get_input_caps = emulator.get_proc_address(
            "libretropy_get_input_device_capabilities", TypedFunctionPointer[c_uint64, []]
        )
        assert get_input_caps is not None
        assert get_input_caps() > 0


@pytest.mark.nds_rom
def test_power_state(session: SessionFactory, nds_rom: Path) -> None:
    """The core reads back exactly the power state the frontend reports."""
    power = retro_device_power(PowerState.DISCHARGING, 3540, 52)

    with session(nds_rom, device_power=power) as emulator:
        get_power = emulator.get_proc_address(
            b"libretropy_get_power", TypedFunctionPointer[c_bool, [TypedPointer[retro_device_power]]]
        )
        assert get_power is not None

        returned_power = retro_device_power()
        assert get_power(byref(returned_power))
        assert power == returned_power


# --------------------------------------------------------------------------- #
# Messages and logging
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_message_version(session: SessionFactory, nds_rom: Path) -> None:
    """The core sees message interface version 1."""
    with session(nds_rom) as emulator:
        get_message_version = emulator.get_proc_address(
            b"libretropy_get_message_version", TypedFunctionPointer[c_uint, []]
        )
        assert get_message_version is not None
        assert get_message_version() == 1


@pytest.mark.nds_sysfiles
@pytest.mark.parametrize("version", [0, 1], ids=["v0", "v1"])
def test_sends_messages(session: SessionFactory, version: int) -> None:
    """A message the core sends reaches the frontend's message driver."""
    driver = LoggerMessageDriver(version, logging.getLogger("libretro"))

    with session(message=driver) as emulator:
        message = emulator.message
        assert message is not None
        assert message.version == version

        send_message = emulator.get_proc_address(
            b"libretropy_send_message", TypedFunctionPointer[c_bool, [CStringArg]]
        )
        assert send_message is not None
        assert send_message(b"Hello, world!")

        # Version 0 lands in `messages`; version 1 lands in `message_exts`.
        received = message.messages if version == 0 else message.message_exts
        assert received is not None
        assert len(received) > 0
        assert any(m.msg == b"Hello, world!" for m in received)


@pytest.mark.nds_sysfiles
def test_logs_output(session: SessionFactory) -> None:
    """The core logs at least something through the frontend's log callback."""
    with session() as emulator:
        log = emulator.log
        assert log is not None
        assert log.records is not None
        assert len(log.records) > 0


# --------------------------------------------------------------------------- #
# Metadata the core declares
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_input_descriptors(session: SessionFactory, nds_rom: Path) -> None:
    """Every declared input descriptor has a description."""
    with session(nds_rom) as emulator:
        descriptors = emulator.input_descriptors

        assert descriptors is not None
        assert len(descriptors) > 0
        assert all(d.description for d in descriptors)


@pytest.mark.nds_rom
def test_controller_info(session: SessionFactory, nds_rom: Path) -> None:
    """Every declared controller has a description, and the data outlives the core."""
    with session(nds_rom) as emulator:
        info = emulator.controller_info

        assert info is not None
        assert len(info) > 0
        assert all(i.desc for i in info)

    # Formatting this after unloading the core proves the data is still valid.
    assert pformat(info)
    assert pformat(info[0])


@pytest.mark.nds_sysfiles
def test_defines_subsystems(session: SessionFactory) -> None:
    """Every declared subsystem has a description, and the data outlives the core."""
    with session() as emulator:
        subsystems = emulator.subsystems

        assert subsystems is not None
        assert len(subsystems) > 0
        assert all(s.desc for s in subsystems)

    # Formatting this after unloading the core proves the data is still valid.
    assert pformat(subsystems)


@pytest.mark.nds_sysfiles
def test_content_info_overrides(session: SessionFactory) -> None:
    """Every declared content info override names at least one extension."""
    with session() as emulator:
        overrides = emulator.content_info_overrides

        assert overrides is not None
        assert len(overrides) > 0
        assert all(o.extensions for o in overrides)


@pytest.mark.nds_rom
def test_achievement_support(session: SessionFactory, nds_rom: Path) -> None:
    """The core registers support for achievements."""
    with session(nds_rom) as emulator:
        assert emulator.support_achievements


@pytest.mark.nds_rom
def test_no_content_support(session: SessionFactory, nds_rom: Path) -> None:
    """The core registers support for running without content."""
    with session(nds_rom) as emulator:
        assert emulator.support_no_game is True


# --------------------------------------------------------------------------- #
# Virtual file system
# --------------------------------------------------------------------------- #


@pytest.mark.dsi_sysfiles
def test_vfs_interface(session: SessionFactory) -> None:
    """
    The core uses the VFS interface the frontend supplied.

    This runs in DSi mode
    because that exercises the file system far more than DS mode does.
    """
    vfs = HistoryFileSystemDriver(DefaultFileSystemDriver())

    with session(vfs=vfs, options={"melonds_console_mode": "dsi"}) as emulator:
        assert emulator.vfs is vfs
        assert vfs.history is not None
        assert len(vfs.history) > 0
