from sys import argv
from libretro import default_session

import prelude

with default_session(argv[1], argv[2]) as session:
    for i in range(10):
        session.core.run()

    size = session.core.serialize_size()
    buffer = bytearray(size)
    state_saved = session.core.serialize(buffer)

    assert state_saved
    assert any(buffer)
