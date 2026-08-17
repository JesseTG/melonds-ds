"""Savestates and the memory regions the core exposes."""

from __future__ import annotations

from ctypes import c_ubyte, cast
from pathlib import Path

import pytest
from libretro import RETRO_MEMORY_SAVE_RAM, RETRO_MEMORY_SYSTEM_RAM
from libretro.ctypes import TypedPointer

from melondsds import SessionFactory


@pytest.mark.nds_rom
def test_saves_state(session: SessionFactory, nds_rom: Path) -> None:
    """``retro_serialize`` produces a non-empty savestate."""
    with session(nds_rom) as emulator:
        for _ in range(10):
            emulator.run()

        size = emulator.core.serialize_size()
        assert size > 0

        buffer = bytearray(size)
        assert emulator.core.serialize(buffer)
        assert any(buffer)


@pytest.mark.nds_rom
def test_saves_and_loads_state(session: SessionFactory, nds_rom: Path) -> None:
    """A savestate taken 30 frames in can be restored 30 frames later."""
    with session(nds_rom) as emulator:
        for _ in range(30):
            emulator.run()

        size = emulator.core.serialize_size()
        assert size > 0

        buffer = bytearray(size)
        assert emulator.core.serialize(buffer)
        assert any(buffer)

        for _ in range(30):
            emulator.run()

        assert emulator.core.serialize_size() == size
        assert emulator.core.unserialize(buffer)


@pytest.mark.nds_rom
def test_exposes_ram(session: SessionFactory, nds_rom: Path) -> None:
    """System RAM is exposed, correctly sized, and writable through the frontend."""
    with session(nds_rom) as emulator:
        size = emulator.core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM)
        assert size is not None
        assert size > 0

        data = emulator.core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM)
        assert data
        assert data.value

        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None
        assert len(memory) == size

        # Make sure we can write to the memory
        memory[0:5] = b"hello"
        mem_ptr = cast(data, TypedPointer[c_ubyte])
        assert bytes(mem_ptr[0:5]) == b"hello"


@pytest.mark.nds_rom
def test_exposes_sram(session: SessionFactory, nds_rom: Path) -> None:
    """Save RAM is exposed and correctly sized."""
    with session(nds_rom) as emulator:
        size = emulator.core.get_memory_size(RETRO_MEMORY_SAVE_RAM)
        assert size is not None
        assert size > 0

        data = emulator.core.get_memory_data(RETRO_MEMORY_SAVE_RAM)
        assert data
        assert data.value
