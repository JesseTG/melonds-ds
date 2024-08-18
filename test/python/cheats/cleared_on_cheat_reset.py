from ctypes import CFUNCTYPE, c_uint

from libretro import Session
from libretro.h import RETRO_MEMORY_SYSTEM_RAM

import prelude

session: Session
with prelude.session() as session:
    memory = session.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)

    assert memory is not None

    num_cheats = session.get_proc_address(b"melondsds_num_cheats", CFUNCTYPE(c_uint))

    session.core.cheat_set(0, True, b'02000000 DEADBEEF')

    cheats = num_cheats()
    assert cheats == 1, f"Expected 1 cheat, got {cheats}"

    for i in range(60):
        session.run()

    session.core.cheat_reset()

    cheats = num_cheats()
    assert cheats == 0, f"Expected 0 cheats, got {cheats}"
