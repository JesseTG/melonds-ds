import itertools
from sys import argv

import libretro.callback.input
from libretro import default_session
from libretro.callback.input import JoypadState


def generate_input():
    yield from itertools.repeat(0, 180)

    yield JoypadState(a=True)

    yield from itertools.repeat(0)


with default_session(argv[1]) as session:

    for i in range(180):
        session.core.run()

    # TODO: Take screenshot
    # TODO: Emulate button press

    for i in range(180):
        session.core.run()
