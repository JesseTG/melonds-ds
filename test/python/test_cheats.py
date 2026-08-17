"""
Applying, resetting and validating cheats.

All of these use the same trivial Action Replay code, ``0XXXXXXX YYYYYYYY``,
which writes the word ``YYYYYYYY`` to address ``XXXXXXX``.
See https://mgba-emu.github.io/gbatek/#dscartcheatactionreplayds for the format.

Cheats aren't applied immediately --
the ARM7 applies them when it processes a VBlank IRQ,
so every case has to run a few frames first.
"""

from __future__ import annotations

from ctypes import c_uint
from pathlib import Path

import pytest
from libretro import RETRO_MEMORY_SYSTEM_RAM, LogLevel
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.options import DIRECT_BOOT_BUILTIN

#: Writes 0xDEADBEEF to 0x02000000.
CHEAT = b"02000000 DEADBEEF"

#: The same value as it appears in memory, read byte-by-byte rather than as a word.
CHEAT_RESULT = b"\xef\xbe\xad\xde"


@pytest.mark.nds_rom
def test_applies_cheats(session: SessionFactory, nds_rom: Path) -> None:
    """An enabled cheat writes its value into system RAM."""
    with session(nds_rom, options=DIRECT_BOOT_BUILTIN) as emulator:
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        emulator.core.cheat_set(0, True, CHEAT)
        assert memory[0:4].tobytes() != CHEAT_RESULT

        for _ in range(60):
            emulator.run()

        assert memory[0:4].tobytes() == CHEAT_RESULT


@pytest.mark.nds_rom
def test_cheats_persist_after_reset(session: SessionFactory, nds_rom: Path) -> None:
    """A cheat survives ``retro_reset`` and is reapplied to the fresh console."""
    with session(nds_rom) as emulator:
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        num_cheats = emulator.get_proc_address(b"melondsds_num_cheats", TypedFunctionPointer[c_uint, []])
        assert num_cheats is not None

        emulator.core.cheat_set(0, True, CHEAT)
        assert memory[0:4].tobytes() != CHEAT_RESULT

        for _ in range(60):
            emulator.run()

        emulator.reset()

        assert num_cheats() == 1

        for _ in range(60):
            emulator.run()

        # Resetting throws the console out, so the old memory buffer is stale.
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory
        assert memory[0:4].tobytes() == CHEAT_RESULT


@pytest.mark.nds_rom
def test_cheat_reset_clears_them(session: SessionFactory, nds_rom: Path) -> None:
    """``retro_cheat_reset`` removes every registered cheat."""
    with session(nds_rom) as emulator:
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        num_cheats = emulator.get_proc_address(b"melondsds_num_cheats", TypedFunctionPointer[c_uint, []])
        assert num_cheats is not None

        emulator.core.cheat_set(0, True, CHEAT)
        assert num_cheats() == 1

        for _ in range(60):
            emulator.run()

        emulator.core.cheat_reset()

        assert num_cheats() == 0


@pytest.mark.nds_rom
def test_invalid_cheat_not_enabled(session: SessionFactory, nds_rom: Path) -> None:
    """A malformed cheat code produces a warning rather than being applied."""
    with session(nds_rom) as emulator:
        emulator.core.cheat_set(0, True, b"fgsfds")

        assert emulator.message.message_exts[-1].level == LogLevel.WARNING
