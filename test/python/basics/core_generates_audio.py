from sys import argv
from libretro import default_session

with default_session(argv[1]) as session:
    for i in range(300):
        session.core.run()

    assert session.audio.buffer is not None
    assert len(session.audio.buffer) > 0

    assert any(b != 0 for b in session.audio.buffer)