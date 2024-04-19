from ctypes import *

from libretro import Session
from libretro.h import RETRO_MEMORY_SYSTEM_RAM

import prelude

session: Session
with prelude.session() as session:
    size = session.core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM)

    assert size is not None
    assert size > 0

    data = session.core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM)

    assert data
    assert data is not None
    assert data.value

    memory = session.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)

    assert memory is not None
    assert len(memory) == size

    # Let's ensure that we can write to the memory
    memory[0:5] = b'hello'
    mem_ptr = cast(data, POINTER(c_ubyte))
    assert bytes(mem_ptr[0:5]) == b'hello'

