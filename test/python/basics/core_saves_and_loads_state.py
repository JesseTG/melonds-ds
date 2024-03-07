from sys import argv
from libretro import default_session

import prelude

with default_session(argv[1], argv[2]) as session:
    for i in range(30):
        session.core.run()

    size = session.core.serialize_size()
    buffer = bytearray(size)
    state_saved = session.core.serialize(buffer)

    assert state_saved
    assert any(buffer)

    for i in range(30):
        session.core.run()

    new_size = session.core.serialize_size()
    assert new_size == size

    state_loaded = session.core.unserialize(buffer)

    assert state_loaded
