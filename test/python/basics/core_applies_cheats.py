from libretro import Session
from libretro.h import RETRO_MEMORY_SYSTEM_RAM

import prelude

session: Session
with prelude.session() as session:
    memory = session.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)

    assert memory is not None

    session.core.cheat_set(0, True, b'02000000 DEADBEEF')
    # A trivial cheat code with one instruction: 0XXXXXXX-YYYYYYYY,
    # where XXXXXXX (in this case, 02000000) is the address to write to
    # and YYYYYYYY (in this case, DEADBEEF) is the value to write.
    # See https://mgba-emu.github.io/gbatek/#dscartcheatactionreplayds for more details

    # Slicing reads the bytes in sequence, not as a word
    assert memory[0:4].tobytes() == b'\xef\xbe\xad\xde'

    session.core.cheat_set(0, False, b'02000000 CAFEBABE')

    assert memory[0:4].tobytes() == b'\xef\xbe\xad\xde'
    # Passing False to enabled shouldn't apply the cheat
