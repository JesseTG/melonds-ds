import itertools
from typing import cast

from libretro import Session, CoreShutDownException
from libretro.api.input import DeviceIdJoypad

import prelude
from libretro.api.video import PillowVideoDriver


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
    video = cast(PillowVideoDriver, session.video)

    for i in range(600):
        session.core.run()
        # The core should shut down at some point during the test

    assert False, "session.core should've raised CoreShutDownException and exited the runtime context"

# noinspection PyUnreachableCode
assert session is not None
assert session.is_shutdown
assert session.is_exited

try:
    session.core.run()
except CoreShutDownException as e:
    pass

print("Core shut down successfully")
