from collections.abc import Iterator
from itertools import repeat
from random import randint

from libretro import Session, MicrophoneDriver, DeviceIdJoypad

import prelude

def generate_noise() -> Iterator[int]:
    while True:
        yield randint(-30000, 30000)

def generate_input():
    yield from repeat(DeviceIdJoypad.L3)

options = {
    "melonds_mic_input": "microphone",
    "melonds_mic_input_active": "hold"
}

session: Session
with prelude.builder().with_mic(generate_noise).with_input(generate_input).with_options(options).build() as session:
    mic = session.mic

    open_mics = len(tuple(m for m in mic.microphones if m.state))
    assert open_mics == 0, f"Expected no open microphones, got {open_mics}"

    session.run()

    open_mics = len(tuple(m for m in mic.microphones if m.state))
    assert open_mics == 1, f"Expected one open microphone, got {open_mics}"
