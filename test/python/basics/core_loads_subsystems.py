from ctypes import CFUNCTYPE, c_bool, c_size_t, c_uint8, c_size_t, POINTER

from libretro import Session

import prelude

assert prelude.subsystem is not None

session: Session
with prelude.session() as session:
    subsystems = session.subsystems

    assert subsystems is not None
    subsystem: bytes = prelude.subsystem.encode()
    idents = [bytes(s.ident) for s in subsystems]
    assert subsystem in idents, f"Subsystem {subsystem} not found in {idents}"

    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    gba_rom_length = session.get_proc_address(b"melondsds_gba_rom_length", CFUNCTYPE(c_size_t))
    assert gba_rom_length is not None, "Core needs to define melondsds_gba_rom_length"
    assert gba_rom_length() > 0, "GBA ROM not installed"

    gba_rom = session.get_proc_address(b"melondsds_gba_rom", CFUNCTYPE(POINTER(c_uint8)))
    assert gba_rom is not None, "Core needs to define melondsds_gba_rom"

    rom = gba_rom()
    assert rom, "GBA ROM not loaded"

    gba_sram_length = session.get_proc_address(b"melondsds_gba_sram_length", CFUNCTYPE(c_size_t))
    assert gba_sram_length is not None, "Core needs to define melondsds_gba_sram_length"
    assert gba_sram_length() > 0, "GBA SRAM not installed"

    gba_sram = session.get_proc_address(b"melondsds_gba_sram", CFUNCTYPE(POINTER(c_uint8)))
    assert gba_sram is not None, "Core needs to define melondsds_gba_sram"

    sram = gba_sram()
    assert sram, "GBA SRAM not loaded"
