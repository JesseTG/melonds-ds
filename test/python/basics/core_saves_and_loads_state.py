from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    for i in range(30):
        session.core.run()

    size = session.core.serialize_size()
    assert size > 0

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
