from typing import cast
from libretro import Session
from libretro.api import ArrayAudioState

import prelude

session: Session
with prelude.session() as session:
    audio = cast(ArrayAudioState, session.audio)
    for i in range(300):
        session.core.run()

    assert audio.buffer is not None
    assert len(audio.buffer) > 0

    assert any(b != 0 for b in audio.buffer)
