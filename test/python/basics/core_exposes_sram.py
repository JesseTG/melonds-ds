from libretro import Session
from libretro.h import RETRO_MEMORY_SAVE_RAM

import prelude

session: Session
with prelude.session() as session:
    size = session.core.get_memory_size(RETRO_MEMORY_SAVE_RAM)

    assert size is not None
    assert size > 0

    data = session.core.get_memory_data(RETRO_MEMORY_SAVE_RAM)

    assert data
    assert data is not None
    assert data.value
