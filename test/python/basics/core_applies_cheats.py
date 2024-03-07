from sys import argv
from libretro import default_session, RETRO_MEMORY_SYSTEM_RAM

with default_session(argv[1]) as session:
    memory = session.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)

    assert memory is not None

    session.core.cheat_set(0, True, b'02000000-DEADBEEF')
    # A trivial cheat code with one instruction: 0XXXXXXX-YYYYYYYY,
    # where XXXXXXX (in this case, 02000000) is the address to write to
    # and YYYYYYYY (in this case, DEADBEEF) is the value to write.
    # See https://mgba-emu.github.io/gbatek/#dscartcheatactionreplayds for more details

    assert memory[0:4].tobytes() == b'\xde\xad\xbe\xef'

    session.core.cheat_set(0, False, b'02000000-CAFEBABE')

    assert memory[0:4].tobytes() == b'\xde\xad\xbe\xef'
    # Passing False to enabled shouldn't apply the cheat
