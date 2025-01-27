from libretro import Session

import prelude

options = {
    "melonds_mic_input": "microphone",
    "melonds_mic_input_active": "always"
}

session: Session
with prelude.builder().with_options(options).build() as session:
    mic = session.mic

    session.run()

    open_mics = len(tuple(m for m in mic.microphones if m.state))
    assert open_mics == 1, f"Expected one open microphone, got {open_mics}"

