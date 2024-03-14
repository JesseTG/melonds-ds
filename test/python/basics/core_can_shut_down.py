import itertools
from typing import cast

from libretro import Session
from libretro.api.input import DeviceIdJoypad
from libretro.api.video import SoftwareVideoState

import prelude


def generate_input():
    # Wait for intro to finish
    yield from itertools.repeat(None, 240)

    # Go to NDS main menu and wait for it to appear
    yield DeviceIdJoypad.A
    yield from itertools.repeat(None, 59)

    # Select the options menu and wait for the cursor to move
    yield DeviceIdJoypad.DOWN
    yield from itertools.repeat(None, 29)

    # Go to the options menu and wait for it to appear
    yield DeviceIdJoypad.A
    yield from itertools.repeat(None, 179)

    # Exit the options menu and wait for the window to appear
    yield DeviceIdJoypad.B
    yield from itertools.repeat(None, 29)

    # Confirm the exit (exiting the NDS options menu shuts down the console)
    yield DeviceIdJoypad.A
    yield from itertools.repeat(None)


session: Session
with prelude.session(input_state=generate_input) as session:
    video = cast(SoftwareVideoState, session.video)
    for i in range(600):
        session.core.run()

    assert session.is_shutdown
