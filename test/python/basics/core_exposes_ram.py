from sys import argv
from libretro import default_session, RETRO_MEMORY_SYSTEM_RAM

with default_session(argv[1]) as session:
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
    assert id(memory) == data.value
