from libretro import Session

import prelude

options = {
    "melonds_mic_input": "microphone"
}

session: Session
with prelude.builder().with_options(options).build() as session:
    mic = session.mic

    assert len(mic.microphones) == 1, f"Expected one open microphone, got {len(mic.microphones)}"

