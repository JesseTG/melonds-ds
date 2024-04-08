from collections.abc import Iterator
from itertools import count, repeat
from math import sin
from typing import cast

from libretro import Session, ArrayAudioDriver, JoypadState

import prelude


def generate_input() -> Iterator[int]:
    yield from repeat(None, 6)
    yield JoypadState(a=True)
    yield from repeat(None, 90)
    yield JoypadState(a=True)
    yield from repeat(None)


def generate_sine_wave() -> Iterator[int]:
    for i in count():
        yield int(sin(i * 440) * 30000)


options = {
    "melonds_mic_input_active": "always"
}

session: Session
with prelude.builder().with_input(generate_input).with_mic(generate_sine_wave).with_options(options).build() as session:
    audio = cast(ArrayAudioDriver, session.audio)
    for i in range(300):
        session.core.run()

    assert audio.buffer is not None
    assert len(audio.buffer) > 0
    assert any(b != 0 for b in audio.buffer)
    # Assert that we're not just being given silence
